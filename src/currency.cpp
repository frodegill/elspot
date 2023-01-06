#include "currency.h"

#include <memory>

#include <fmt/printf.h>

#include <Poco/JSON/Parser.h>

#include "application.h"


bool Currency::GetCurrentExchangeRate(double& rate)
{
  return GetExchangeRate(UTCTime().AsNorwegianDay(), rate);
}

bool Currency::GetExchangeRate(const NorwegianDay& norwegian_day, double& rate)
{
  std::map<unsigned long, double>::iterator existing_rate;
  
  { //Lock scope
    const std::lock_guard<std::mutex> lock(m_rates_mutex);

    //Already fetched?
    existing_rate = m_rates.find(norwegian_day.AsULong());
    if (existing_rate != m_rates.end())
    {
      rate = existing_rate->second;
      return true;
    }
    
    //Fetch rate!
    if (!FetchEur(norwegian_day))
        return false;

    //Has it been fetched?
    existing_rate = m_rates.find(norwegian_day.AsULong());
    if (existing_rate == m_rates.end())
    {
      return false;
    }
  }
  
  //It has been fetched! Return it
  rate = (*existing_rate).second;
  return true;
}

bool Currency::FetchEur(const NorwegianDay& norwegian_day)
{
  //Remove expired failures
  auto fail_expire_time = std::chrono::system_clock::now() - RETRY_DURATION;

  { //Lock scope
    const std::lock_guard<std::mutex> lock(m_failmap_mutex);

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
      return false;
    }
  }

  try
  {
#if 1 //Actually fetch exchange rate. There is a limit on free requests per month
    Poco::URI uri;
    NorwegianDay norwegian_today = UTCTime().AsNorwegianDay();
    if (norwegian_day < norwegian_today)
    {
      uri = Poco::URI(fmt::sprintf(EUR_HISTORICAL_URL,
                                   norwegian_day.GetYear(), norwegian_day.GetMonth(), norwegian_day.GetDay(),
                                   ::GetApp()->GetConfig(Elspot::EXCHANGERATESAPI_TOKEN_PROPERTY)));
    }
    else
    {
      uri = Poco::URI(fmt::sprintf(EUR_LATEST_URL, ::GetApp()->GetConfig(Elspot::EXCHANGERATESAPI_TOKEN_PROPERTY)));
    }

    const std::shared_ptr<Networking> networking = ::GetApp()->GetNetworking();
    if (!networking.get())
    {
      return false;
    }
    std::shared_ptr<Poco::Net::HTTPSClientSession> session = networking->CreateSession(uri);
    networking->CallGET(session, uri, "application/json");

    Poco::Net::HTTPResponse res;
    Poco::JSON::Parser parser;
    auto json_root = parser.parse(session->receiveResponse(res));
    if (Poco::Net::HTTPResponse::HTTP_OK != res.getStatus())
    {
      return RegisterFail(norwegian_day);
    }

    if (!json_root)
    {
      return RegisterFail(norwegian_day);
    }

    auto object_root = json_root.extract<Poco::JSON::Object::Ptr>();
    if (!object_root)
    {
      return RegisterFail(norwegian_day);
    }

    std::string base_currency = object_root->getValue<std::string>("base");
    if (0 != base_currency.compare("EUR"))
    {
      return RegisterFail(norwegian_day);
    }

    Poco::JSON::Object::Ptr rates_object = object_root->getObject("rates");
    if (!rates_object)
    {
      return RegisterFail(norwegian_day);
    }

    //Remove any old values
#if 0
    std::erase_if(m_rates, [norwegian_day](const auto& item)
      {
        auto const& [key, value] = item;
        return norwegian_day.DaysAfter(key) > 1;
      });
#else
    for (std::map<unsigned long, double>::iterator it=m_rates.begin(); it!=m_rates.end(); ++it)
    {
        if (norwegian_day.DaysAfter(it->first) > 1)
        {
          m_rates.erase(it);
        }
    }
#endif

    double exchange_rate = rates_object->getValue<double>("NOK");
    m_rates.insert({norwegian_day.AsULong(), exchange_rate});
#else
    Poco::Logger::get(Logger::DEFAULT).information("Currency::FetchEur hardcoding 10.2");
    m_rates.insert({norwegian_day.AsULong(), 10.2});
#endif
    return true;
  }
  catch (Poco::Exception& ex)
  {
    Poco::Logger::get(Logger::DEFAULT).error(ex.message());
    return RegisterFail(norwegian_day);
  }
  catch (...)
  {
    Poco::Logger::get(Logger::DEFAULT).error("Got currency exception");
    return RegisterFail(norwegian_day);
  }
}

bool Currency::RegisterFail(const NorwegianDay& norwegian_day)
{
  { //Lock scope
    const std::lock_guard<std::mutex> lock(m_failmap_mutex);

    Poco::Logger::get(Logger::DEFAULT).error(std::string("Fetching exchange rate failed for ")+norwegian_day.ToString());
    m_failmap.insert({norwegian_day.AsULong(), std::chrono::system_clock::now()});
  }
  
  return false;
}
