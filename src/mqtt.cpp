#include "mqtt.h"

#include <algorithm>
#include <iomanip>
#include <sstream>

#include <fmt/printf.h>

#include "application.h"


MQTT::MQTT()
{
  m_mqtt_client = std::make_unique<mqtt::client>(::GetApp()->GetConfig("mqtt_server"), CLIENT_ID, MAX_BUFFERED_MESSAGES);
	m_mqtt_client->set_callback(*this);

  auto connopts = mqtt::connect_options_builder()
                    .clean_session(true)
                    .keep_alive_interval(std::chrono::seconds(20))
                    .automatic_reconnect(true);
  
  if (!::GetApp()->GetConfig("mqtt_username").empty())
  {
    Poco::Logger::get(Logger::DEFAULT).information("Using MQTT username/password");
    connopts.user_name(::GetApp()->GetConfig("mqtt_username"))
            .password(::GetApp()->GetConfig("mqtt_password"));
  }

  if (!::GetApp()->GetConfig("mqtt_keystore").empty())
  {
    Poco::Logger::get(Logger::DEFAULT).information("Using MQTT SSL");
    auto sslopts = mqtt::ssl_options_builder()
                         .trust_store(::GetApp()->GetConfig("mqtt_truststore"))
                         .key_store(::GetApp()->GetConfig("mqtt_keystore"))
                         .error_handler([](const std::string& msg) {std::cerr << "SSL Error: " << msg << std::endl;})
                         .finalize();
             
    connopts.ssl(std::move(sslopts));
  }
  m_connection_options = connopts.finalize();
}

void MQTT::connection_lost(const std::string& cause)
{
  Poco::Logger::get(Logger::DEFAULT).warning(std::string("MQTT Connection lost: ")+cause);
}

bool MQTT::GotPrices(const NorwegianDay& norwegian_day)
{
  try
  { //Lock scope
    const std::lock_guard<std::mutex> lock(MQTT::m_connection_mutex);

    bool is_today = norwegian_day.IsToday();
    if (!is_today && !norwegian_day.IsTomorrow())
    {
      Poco::Logger::get(Logger::DEFAULT).error(norwegian_day.ToString()+std::string(" is not today/tomorrow"));
      return false;
    }

    Spotprice::AreaQuarterRateType area_rates;
    double exchange_rate;
    if (!GetInfo(norwegian_day, area_rates, exchange_rate))
    {
      Poco::Logger::get(Logger::DEFAULT).information("MQTT GotPrices failed at GetInfo");
      return false;
    }

    bool was_connected = m_mqtt_client->is_connected();
    if (!was_connected)
    {
      m_mqtt_client->connect(m_connection_options);
    }

    bool status = Publish(is_today ? "nordpool/today/exchangerate" : "nordpool/tomorrow/exchangerate", exchange_rate);

    Spotprice::QuarterRateType eur_quarter_rates;
    std::array<HourPrice,Spotprice::HOURS_PER_DAY> sorted_hour_prices;
    std::array<QuarterPrice,Spotprice::QUARTERS_PER_DAY> sorted_quarter_prices;
    for (std::array<Area,5>::size_type area_index=0; area_index<area_rates.size(); area_index++)
    {
      eur_quarter_rates = area_rates[area_index];
      CopyAndSortHourRates(eur_quarter_rates, sorted_hour_prices);
      CopyAndSortQuarterRates(eur_quarter_rates, sorted_quarter_prices);

      for (unsigned int hour=0; hour<Spotprice::HOURS_PER_DAY; hour++)
      {
        status &= Publish(fmt::sprintf(is_today ? "nordpool/today/%s/nok%02d" : "nordpool/tomorrow/%s/nok%02d",
          Spotprice::m_areas[area_index].id, hour),
          Spotprice::GetHourRate(eur_quarter_rates,hour) * exchange_rate);
        status &= Publish(fmt::sprintf(is_today ? "nordpool/today/%s/eur%02d" : "nordpool/tomorrow/%s/eur%02d",
          Spotprice::m_areas[area_index].id, hour),
          Spotprice::GetHourRate(eur_quarter_rates,hour));
        status &= Publish(fmt::sprintf(is_today ? "nordpool/today/%s/order%02d" : "nordpool/tomorrow/%s/order%02d",
          Spotprice::m_areas[area_index].id, hour),
          fmt::sprintf("%d", std::lower_bound(sorted_hour_prices.begin(), sorted_hour_prices.end(), Spotprice::GetHourRate(eur_quarter_rates,hour), [](const HourPrice& a, double b) {return a.price > b;}) - sorted_hour_prices.begin()));
        status &= Publish(fmt::sprintf(is_today ? "nordpool/today/%s/sorted%d" : "nordpool/tomorrow/%s/sorted%d", Spotprice::m_areas[area_index].id, hour),
          fmt::sprintf("%02d", sorted_hour_prices[hour].hour));
      }

      for (unsigned int quarter=0; quarter<Spotprice::QUARTERS_PER_DAY; quarter++)
      {
        status &= Publish(fmt::sprintf(is_today ? "nordpool/today/%s/nok%02d%02d" : "nordpool/tomorrow/%s/nok%02d%02d",
          Spotprice::m_areas[area_index].id, quarter/4, (quarter%4)*15),
          Spotprice::GetQuarterRate(eur_quarter_rates,quarter) * exchange_rate);
        status &= Publish(fmt::sprintf(is_today ? "nordpool/today/%s/eur%02d%02d" : "nordpool/tomorrow/%s/eur%02d%02d",
          Spotprice::m_areas[area_index].id, quarter/4, (quarter%4)*15),
          Spotprice::GetQuarterRate(eur_quarter_rates,quarter));
        status &= Publish(fmt::sprintf(is_today ? "nordpool/today/%s/order%02d%02d" : "nordpool/tomorrow/%s/order%02d%02d",
          Spotprice::m_areas[area_index].id, quarter/4, (quarter%4)*15),
          fmt::sprintf("%d", std::lower_bound(sorted_quarter_prices.begin(), sorted_quarter_prices.end(), Spotprice::GetQuarterRate(eur_quarter_rates,quarter), [](const QuarterPrice& a, double b) {return a.price > b;}) - sorted_quarter_prices.begin()));
        status &= Publish(fmt::sprintf(is_today ? "nordpool/today/%s/sortedq%d" : "nordpool/tomorrow/%s/sortedq%d",
          Spotprice::m_areas[area_index].id, quarter),
          fmt::sprintf("%02d", sorted_quarter_prices[quarter].quarter));
      }
    }

    if (is_today)
    {
      status &= PublishCurrentPrices();
    }
    
    if (!was_connected)
    {
      m_mqtt_client->disconnect();
    }

    return status;
  }
  catch (const mqtt::exception& exc)
  {
    Poco::Logger::get(Logger::DEFAULT).error(exc.get_message());
    return false;
  }
}

bool MQTT::PublishCurrentPrices()
{
  try
  {
    NorwegianTime norwegian_now = UTCTime().AsNorwegianTime();
    Spotprice::AreaQuarterRateType area_quarter_rates;
    double exchange_rate;
    if (!GetInfo(norwegian_now, area_quarter_rates, exchange_rate))
    {
      return false;
    }

    bool was_connected = m_mqtt_client->is_connected();
    if (!was_connected)
    {
      m_mqtt_client->connect(m_connection_options);
    }

    Spotprice::QuarterRateType eur_quarter_rates;
    std::array<HourPrice,Spotprice::HOURS_PER_DAY> sorted_hour_prices;
    std::array<QuarterPrice,Spotprice::QUARTERS_PER_DAY> sorted_quarter_prices;
    bool status = true;
    for (std::array<Area,5>::size_type area_index=0; area_index<area_quarter_rates.size(); area_index++)
    {
      eur_quarter_rates = area_quarter_rates[area_index];
      CopyAndSortHourRates(eur_quarter_rates, sorted_hour_prices);
      CopyAndSortQuarterRates(eur_quarter_rates, sorted_quarter_prices);

      status &= Publish(fmt::sprintf("nordpool/today/%s/nok",
        Spotprice::m_areas[area_index].id), Spotprice::GetQuarterRate(eur_quarter_rates, norwegian_now.GetQuarter()) * exchange_rate);
      status &= Publish(fmt::sprintf("nordpool/today/%s/eur",
        Spotprice::m_areas[area_index].id), Spotprice::GetQuarterRate(eur_quarter_rates, norwegian_now.GetQuarter()));
      status &= Publish(fmt::sprintf("nordpool/today/%s/order",
        Spotprice::m_areas[area_index].id),
              fmt::sprintf("%d", std::lower_bound(sorted_hour_prices.begin(), sorted_hour_prices.end(), Spotprice::GetHourRate(eur_quarter_rates, norwegian_now.GetHour()), [](const HourPrice& a, double b) {return a.price > b;}) - sorted_hour_prices.begin()));
      status &= Publish(fmt::sprintf("nordpool/today/%s/orderq",
        Spotprice::m_areas[area_index].id),
              fmt::sprintf("%d", std::lower_bound(sorted_quarter_prices.begin(), sorted_quarter_prices.end(), Spotprice::GetQuarterRate(eur_quarter_rates, norwegian_now.GetQuarter()), [](const QuarterPrice& a, double b) {return a.price > b;}) - sorted_quarter_prices.begin()));
    }

    if (!was_connected)
    {
      m_mqtt_client->disconnect();
    }
    
    return status;
  }
  catch (const mqtt::exception& exc)
  {
    Poco::Logger::get(Logger::DEFAULT).error(exc.get_message());
    return false;
  }
}

bool MQTT::Publish(const std::string& topic, const double& value, int precision)
{
  std::string value_str;
  return Publish(topic, DoubleToString(value, precision));
}

bool MQTT::Publish(const std::string& topic, const std::string& value)
{
  auto msg = mqtt::make_message(topic, value, mqtt::message::DFLT_QOS, true);
  m_mqtt_client->publish(msg);
  return true;
}

bool MQTT::GetInfo(const NorwegianDay& norwegian_day, Spotprice::AreaQuarterRateType& area_quarter_rates, double& exchange_rate) const
{
  if (!::GetApp()->GetSpotprice()->GetEurQuarterRates(norwegian_day, area_quarter_rates))
  {
    Poco::Logger::get(Logger::DEFAULT).error(std::string("Failed to get EUR rates for ") + norwegian_day.ToString());
    return false;
  }

  if (!::GetApp()->GetCurrency()->GetExchangeRate(norwegian_day, exchange_rate))
  {
    Poco::Logger::get(Logger::DEFAULT).error(std::string("Failed to get exchange rate for ") + norwegian_day.ToString());
    return false;
  }
  
  return true;
}

void MQTT::CopyAndSortHourRates(const Spotprice::QuarterRateType& eur_rates, std::array<HourPrice,Spotprice::HOURS_PER_DAY>& sorted_hour_prices) const
{
  //Copy and sort
  for (unsigned int hour=0; hour<Spotprice::HOURS_PER_DAY; hour++)
  {
    sorted_hour_prices[hour].hour = hour;
    sorted_hour_prices[hour].price = Spotprice::GetHourRate(eur_rates, hour);
  }
  std::sort(sorted_hour_prices.begin(), sorted_hour_prices.end(), [](const HourPrice& a, const HourPrice& b) {return a.price > b.price;});
}

void MQTT::CopyAndSortQuarterRates(const Spotprice::QuarterRateType& eur_rates, std::array<QuarterPrice,Spotprice::QUARTERS_PER_DAY>& sorted_quarter_prices) const
{
  //Copy and sort
  for (unsigned int quarter=0; quarter<Spotprice::QUARTERS_PER_DAY; quarter++)
  {
    sorted_quarter_prices[quarter].quarter = quarter;
    sorted_quarter_prices[quarter].price = Spotprice::GetQuarterRate(eur_rates, quarter);
  }
  std::sort(sorted_quarter_prices.begin(), sorted_quarter_prices.end(), [](const QuarterPrice& a, const QuarterPrice& b) {return a.price > b.price;});
}

std::string MQTT::DoubleToString(const double& value, int precision)
{
  std::ostringstream stream;
  stream << std::fixed;
  stream << std::setprecision(precision);
  stream << value;
  return stream.str();
}
