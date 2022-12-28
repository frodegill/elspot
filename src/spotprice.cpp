#include "spotprice.h"

#include <chrono>
#include <fstream>
#include <sstream>
#include <thread>

#include <fmt/printf.h>

#include <Poco/Logger.h>
#include <Poco/DOM/AutoPtr.h>
#include <Poco/DOM/Document.h>
#include <Poco/DOM/DOMParser.h>
#include <Poco/DOM/NodeList.h>
#include <Poco/SAX/InputSource.h>

#include "application.h"


bool Spotprice::HasEurRate(const NorwegianDay& norwegian_day)
{
  { //Lock scope
    const std::lock_guard<std::mutex> lock(m_eur_rates_mutex);

    return m_eur_rates.find(norwegian_day.AsULong()) != m_eur_rates.end();
  }
}

bool Spotprice::CacheEurRates(const NorwegianDay& norwegian_day)
{
  AreaRateType dummy;
  return GetEurRates(norwegian_day, dummy);
}

bool Spotprice::GetEurRates(const NorwegianDay& norwegian_day, AreaRateType& eur_rates)
{
  { //Lock scope
    const std::lock_guard<std::mutex> lock(m_eur_rates_mutex);

    //Already fetched?
    auto existing_rate = m_eur_rates.find(norwegian_day.AsULong());
    if (existing_rate != m_eur_rates.end())
    {
      eur_rates = existing_rate->second;
      return true;
    }
    
    //Fetch rate!
    if (!FetchEurRates(norwegian_day))
        return false;

    //Has it been fetched?
    existing_rate = m_eur_rates.find(norwegian_day.AsULong());
    if (existing_rate == m_eur_rates.end())
    {
      return false;
    }
  
    //It has been fetched! Return it
    eur_rates = (*existing_rate).second;
  }
  return true;
}

bool Spotprice::FetchEurRates(const NorwegianDay& norwegian_day)
{
  { //Lock scope
    const std::lock_guard<std::mutex> lock(m_failmap_mutex);

    //Remove expired failures
    auto fail_expire_time = std::chrono::system_clock::now() - RETRY_DURATION;
#if 0
    std::erase_if(m_failmap, [fail_expire_time](const auto& item) {
      auto const& [key, value] = item;
      return value < fail_expire_time;
    });
#else
    for (std::map<unsigned long, std::chrono::system_clock::time_point>::iterator it=m_failmap.begin(); it!=m_failmap.end(); ++it)
    {
        if (it->second < fail_expire_time)
        {
          m_failmap.erase(it);
        }
    }
#endif

    if (m_failmap.find(norwegian_day.AsULong()) != m_failmap.end()) //Recently failed?
    {
      Poco::Logger::get(Logger::DEFAULT).information(std::string("Recently failed fetching spotprice for day ")+norwegian_day.ToString());
      return false;
    }
  }

  std::string xml_buffer;
  Poco::XML::NodeList* points = nullptr;
  try
  {
    AreaRateType area_rates;
    
    for (std::array<Area,5>::size_type area_index=0; area_index<m_areas.size(); area_index++)
    {
      Poco::URI uri(fmt::sprintf(DAYAHEAD_URL, ::GetApp()->GetConfig(Elspot::ENTSOE_TOKEN_PROPERTY), m_areas[area_index].code, m_areas[area_index].code, norwegian_day.AsULong(), norwegian_day.AsULong()));

      const std::shared_ptr<Networking> networking = ::GetApp()->GetNetworking();
      if (!networking.get())
      {
        return false;
      }
      std::unique_ptr<Poco::Net::HTTPSClientSession> session = networking->CreateSession(uri);
      networking->CallGET(session, uri, "application/xml");

      DayRateType area_prices;
      Poco::Net::HTTPResponse res;
      Poco::XML::DOMParser dom_parser;
      Poco::XML::InputSource xml_src(session->receiveResponse(res));
      if (Poco::Net::HTTPResponse::HTTP_OK != res.getStatus())
      {
        return RegisterFail(norwegian_day);
      }

      xml_buffer = std::string(std::istreambuf_iterator<char>(*xml_src.getByteStream()), {});
      Poco::AutoPtr<Poco::XML::Document> xml_doc = dom_parser.parseString(xml_buffer);
      points = xml_doc->getElementsByTagName("Point");
      if (points->length()<23 || points->length()>25)
      {
        Poco::Logger::get(Logger::DEFAULT).error(std::string("Spotprice: Didn't get 24 +/- 1 points for ")+norwegian_day.ToString());
        points->release();
        return RegisterFail(norwegian_day);
      }
      
      bool winter_to_summertime = points->length()==23;
      bool summer_to_wintertime = points->length()==23;
      int offset = 0;
      for (unsigned long point_index=0; point_index<points->length(); point_index++)
      {
        Poco::XML::Node* point = points->item(point_index);
        if (point->nodeType() == Poco::XML::Node::ELEMENT_NODE)
        {
          Poco::XML::Element* position = static_cast<Poco::XML::Element*>(point)->getChildElement("position");
          Poco::XML::Element* price_amount = static_cast<Poco::XML::Element*>(point)->getChildElement("price.amount");
          if (position && price_amount)
          {
            std::string position_value = position->innerText();
            std::string price_amount_value = price_amount->innerText();
            if (!position_value.empty() && !price_amount_value.empty())
            {
              int position_index = std::stoi(position_value) - 1; //Position_value is [1-24], unless change to summer- or wintertime
              double price = std::stod(price_amount_value);
              if (winter_to_summertime)
              {
                offset = position_index>1 ? 1 : 0;
              }
              else if (summer_to_wintertime)
              {
                offset = position_index>1 ? -1 : 0;
              }

              if (0<=(position_index+offset) && HOURS_PER_DAY>(position_index+offset))
              {
                area_prices[position_index+offset] = price;

                if (winter_to_summertime && position_index==1)
                {
                  area_prices[position_index+1] = price;
                }
              }
            }
          }
        }
      }
      area_rates[area_index] = area_prices;

      points->release();
      points = nullptr;
    }

    m_eur_rates[norwegian_day.AsULong()] = area_rates;
    
    Poco::Logger::get(Logger::DEFAULT).information(std::string("Spotprice: Got all prices for ")+norwegian_day.ToString());
    return true;
  }
  catch (Poco::Exception& ex)
  {
    Poco::Logger::get(Logger::DEFAULT).error(ex.message());
    if (!xml_buffer.empty())
    {
      Poco::Logger::get(Logger::DEFAULT).error(xml_buffer);
    }
    
    if (points)
    {
      points->release();
    }
  }
  catch (...)
  {
    Poco::Logger::get(Logger::DEFAULT).error("Got spotprice exception");
    if (points)
    {
      points->release();
    }
  }
  return RegisterFail(norwegian_day);
}

bool Spotprice::RegisterFail(const NorwegianDay& norwegian_day)
{
  { //Lock scope
    const std::lock_guard<std::mutex> lock(m_failmap_mutex);

    m_failmap.insert({norwegian_day.AsULong(), std::chrono::system_clock::now()});
  }
  
  return false;
}
