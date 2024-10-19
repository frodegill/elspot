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


bool Spotprice::HasEurRate(const NorwegianDay& norwegian_day) const
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
      std::shared_ptr<Poco::Net::HTTPSClientSession> session = networking->CreateSession(uri);
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
      if (xml_buffer.empty())
      {
        Poco::Logger::get(Logger::DEFAULT).error(std::string("Spotprice: Got 0 bytes for ")+uri.toString());
        return RegisterFail(norwegian_day);
      }

      Poco::AutoPtr<Poco::XML::Document> xml_doc = dom_parser.parseString(xml_buffer);

      //Check curveType
      Poco::XML::NodeList* curve_types = xml_doc->getElementsByTagName("curveType");
      if (curve_types==nullptr || curve_types->length()==0)
      {
        Poco::Logger::get(Logger::DEFAULT).information(std::string("Did not get a curveType"));
      } else {
        Poco::XML::Node* curve_type = curve_types->item(0);
        std::string curve_type_value = curve_type->innerText();
        if (curve_type_value!="A01" && curve_type_value!="A03")
        {
          Poco::Logger::get(Logger::DEFAULT).information(std::string("Got curveType "+curve_type_value));
        }
      }
      if (curve_types) curve_types->release();

      //Check resolution
      Poco::XML::NodeList* resolutions = xml_doc->getElementsByTagName("resolution");
      if (resolutions==nullptr || resolutions->length()==0)
      {
        Poco::Logger::get(Logger::DEFAULT).information(std::string("Did not get a resolution"));
      } else {
        Poco::XML::Node* resolution = resolutions->item(0);
        std::string resolution_value = resolution->innerText();
        if (resolution_value!="PT60M")
        {
          Poco::Logger::get(Logger::DEFAULT).information(std::string("Got resolution "+resolution_value));
        }
      }
      if (resolutions) resolutions->release();

      //Find start time
      std::string start_value;
      Poco::XML::NodeList* starts = xml_doc->getElementsByTagName("start");
      if (starts==nullptr || starts->length()==0)
      {
        Poco::Logger::get(Logger::DEFAULT).error(std::string("Did not get a start time"));
      } else {
        Poco::XML::Node* start = starts->item(0);
        start_value = start->innerText();
      }
      if (starts) starts->release();
      if (start_value.empty())
      {
        return RegisterFail(norwegian_day);
      }
      UTCTime start_time_utc(start_value);

      points = xml_doc->getElementsByTagName("Point");
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
              unsigned int position_hour = std::stoi(position_value) - 1;
              double price = std::stod(price_amount_value);

              //Both curveType A01 and A03 can work if we fill array from current position and to the end
              for (; position_hour<(points->length()); position_hour++)
              {
                NorwegianTime local_time = start_time_utc.IncrementHoursCopy(position_hour).AsNorwegianTime();
                if (norwegian_day == local_time)
                {
                  area_prices[local_time.GetHour()] = price;
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
