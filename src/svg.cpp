#include "svg.h"

#include <cstring>
#include <fstream>
#include <cmath>
#include <sstream>
#include <thread>

#include <fmt/printf.h>

#include <boost/algorithm/string/replace.hpp>

#include "application.h"

#define FLOAT_MARGIN_OF_ERROR (0.0001f)

bool SVG::GenerateSVGs(const NorwegianDay& norwegian_day) const
{
  if (!norwegian_day.IsToday() && !norwegian_day.IsTomorrow())
    return true; //Nothing to do

  //Read SVG template
  auto ss = std::ostringstream{};
  std::ifstream input_file(::GetApp()->GetConfig(Elspot::SVG_TEMPLATE_FILE));
  if (!input_file.is_open()) {
    Poco::Logger::get(Logger::DEFAULT).error("Could not open SVG template file");
    return false;
  }
  ss << input_file.rdbuf();
  std::string svg_template = ss.str();

  //Get prices
  Spotprice::AreaQuarterRateType area_quarter_rates;
  if (!::GetApp()->GetSpotprice()->GetEurQuarterRates(norwegian_day, area_quarter_rates))
    return false;

  double exchange_rate;
  bool found_nok = ::GetApp()->GetCurrency()->GetExchangeRate(norwegian_day, exchange_rate);

  bool status = true;
  for (std::array<Area,5>::size_type area_index=0; area_index<area_quarter_rates.size(); area_index++)
  {
    status &= GenerateSVG(svg_template, norwegian_day, "EUR", 1.0, area_quarter_rates, area_index);

    if (found_nok)
    {
      status &= GenerateSVG(svg_template, norwegian_day, "NOK", exchange_rate, area_quarter_rates, area_index);
    }
  }
  return status;
}

/* All applications has an ugly part. For this application, this is it. Sorry. */
bool SVG::GenerateSVG(const std::string& svg_template, const NorwegianDay& norwegian_day, const std::string& currency_name, const double& exchange_rate, const Spotprice::AreaQuarterRateType& area_quarter_rates, const std::array<Area,5>::size_type& area_index) const
{
  std::string svg_content = svg_template;
  
  boost::replace_all(svg_content, "{day}", std::to_string(norwegian_day.AsULong()));
  boost::replace_all(svg_content, "{currency}", currency_name);
  boost::replace_all(svg_content, "{zone-id}", Spotprice::m_areas[area_index].id);
  boost::replace_all(svg_content, "{zone-description}", Spotprice::m_areas[area_index].name);
  boost::replace_all(svg_content, "{date}", norwegian_day.ToString());
  
  //Find min/max
  double min_rate=0.0, max_rate=INT_MIN, current_rate;
  for (std::array<Area,5>::size_type min_max_index=0; min_max_index<area_quarter_rates.size(); min_max_index++)
  {
    Spotprice::QuarterRateType eur_quarter_rates = area_quarter_rates[min_max_index];
    for (unsigned int quarter=0; quarter<Spotprice::QUARTERS_PER_DAY; quarter++)
    {
      current_rate = Spotprice::GetQuarterRate(eur_quarter_rates, quarter) * exchange_rate;
      if (current_rate < min_rate)
      {
        min_rate = current_rate;
      }
      if (current_rate > max_rate)
      {
        max_rate = current_rate;
      }
      
      if (min_max_index==area_index && (quarter%4)==0)
      {
        unsigned int hour = quarter/4;
        boost::replace_all(svg_content,
                           fmt::sprintf("{hour%d}", hour%Spotprice::HOURS_PER_DAY),
                           MQTT::DoubleToString(Spotprice::GetHourRate(eur_quarter_rates, hour)*exchange_rate, 2));
      }
    }
  }
  double delta_rate = max_rate - min_rate;
  int precision = 3 - ((delta_rate == 0) ? 0 : static_cast<int>(::log10(delta_rate)));

  static constexpr const int grid_steps = 6;
  double ygrid_max = dceil(max_rate, precision-1);
  double ygrid_min = dfloor(min_rate, precision-1);
  for (unsigned int ygrid=0; ygrid<=grid_steps; ygrid++)
  {
    boost::replace_all(svg_content, fmt::sprintf("{y%d}", ygrid), MQTT::DoubleToString(ygrid_min + ygrid*(ygrid_max-ygrid_min)/grid_steps, precision));
  }

  //Draw price lines
  int other_index = 0;
  for (std::array<Area,5>::size_type line_index=0; line_index<area_quarter_rates.size(); line_index++)
  {
    std::stringstream ss;

    Spotprice::QuarterRateType eur_quarter_rates = area_quarter_rates[line_index];
    double previous_rate = Spotprice::GetQuarterRate(eur_quarter_rates, 0) * exchange_rate;
    ss << "M50 " << rateToYPos(previous_rate, min_rate, max_rate);
    
    for (unsigned int quarter=0; quarter<Spotprice::QUARTERS_PER_DAY; quarter++)
    {
      current_rate = Spotprice::GetQuarterRate(eur_quarter_rates, quarter) * exchange_rate;
      if (::fabs(current_rate - previous_rate) > FLOAT_MARGIN_OF_ERROR) {
        ss << " V" << rateToYPos(current_rate, min_rate, max_rate);
      }
      ss << " h6.25";
      previous_rate = current_rate;  
    }
    
    if (area_index == line_index)
    {
      boost::replace_all(svg_content, "{quarter}", ss.str());

      std::stringstream ss_hour;
      previous_rate = Spotprice::GetHourRate(eur_quarter_rates, 0) * exchange_rate;
      ss_hour << "M50 " << rateToYPos(previous_rate, min_rate, max_rate);
      for (unsigned int hour=0; hour<Spotprice::HOURS_PER_DAY; hour++)
      {
        current_rate = Spotprice::GetHourRate(eur_quarter_rates, hour) * exchange_rate;
        if (::fabs(current_rate - previous_rate) > FLOAT_MARGIN_OF_ERROR) {
          ss_hour << " V" << rateToYPos(current_rate, min_rate, max_rate);
        }
        ss_hour << " h25";
        previous_rate = current_rate;
      }
      boost::replace_all(svg_content, "{hour}", ss_hour.str());
    }
    else
    {
      other_index++;
      boost::replace_all(svg_content, fmt::sprintf("{other%d}", other_index), ss.str());
    }
  }

  //Write to file
  std::string filename = fmt::sprintf(norwegian_day.IsToday() ? "%s/today-%s-%s.svg" : "%s/tomorrow-%s-%s.svg",
                    ::GetApp()->GetConfig(Elspot::SVG_DIRECTORY_PROPERTY),
                    Spotprice::m_areas[area_index].id,
                    currency_name);

  std::ofstream svg_file;
  svg_file.open(filename);
  svg_file << svg_content;
  svg_file.close();
  Poco::Logger::get(Logger::DEFAULT).information(std::string("SVG wrote file ")+filename);
  return true;
}

double SVG::dceil(double v, int p) const
{
  v *= pow(10, p);
  v = ceil(v);
  v /= pow(10, p);
  return v;
}

double SVG::dfloor(double v, int p) const
{
  v *= pow(10, p);
  v = floor(v);
  v /= pow(10, p);
  return v;
}

double SVG::rateToYPos(const double& rate, const double& min_rate, const double& max_rate) const
{
  static constexpr const int max_y_linepos = 100;
  static constexpr const int min_y_linepos = 380;
  if (min_rate>=max_rate)
  {
    return min_y_linepos;
  }
  return min_y_linepos + (max_y_linepos-min_y_linepos) * ((rate-min_rate)/(max_rate-min_rate));
}
