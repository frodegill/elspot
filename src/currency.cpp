#include "currency.h"

#include <memory>

#include <fmt/printf.h>

#include <Poco/JSON/Parser.h>
#include <Poco/Net/HTTPClientSession.h>
#include <Poco/Net/HTTPRequest.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/URI.h>

#include "application.h"


bool Currency::GetCurrentExchangeRate(double& rate)
{
  return GetExchangeRate(UTCTime().AsLocalDay(), rate);
}

bool Currency::GetExchangeRate(const LocalDay& day, double& rate)
{
  std::map<unsigned long, double>::iterator existing_rate;
  
  { //Lock scope
    const std::lock_guard<std::mutex> lock(m_rates_mutex);

    //Already fetched?
    existing_rate = m_rates.find(day.AsULong());
    if (existing_rate != m_rates.end())
    {
      rate = existing_rate->second;
      return true;
    }
    
    //Fetch rate!
    if (!FetchEur(day))
        return false;

    //Has it been fetched?
    existing_rate = m_rates.find(day.AsULong());
    if (existing_rate == m_rates.end())
    {
      return false;
    }
  }
  
  //It has been fetched! Return it
  rate = (*existing_rate).second;
  return true;
}

bool Currency::FetchEur(const LocalDay& day)
{
  //Remove expired failures
  auto fail_expire_time = std::chrono::system_clock::now() - RETRY_DURATION;

  { //Lock scope
    const std::lock_guard<std::mutex> lock(m_failmap_mutex);
    
    std::erase_if(m_failmap, [fail_expire_time](const auto& item) {
      auto const& [key, value] = item;
      return value < fail_expire_time;
    });

    if (m_failmap.find(day.AsULong()) != m_failmap.end()) //Recently failed?
    {
      return false;
    }
  }

  try
  {
#if 1 //Actually fetch exchange rate. There is a limit on free requests per month
    Poco::URI uri;
    LocalDay today = UTCTime().AsLocalDay();
    if (day < today)
    {
      uri = Poco::URI(fmt::sprintf(EUR_HISTORICAL_URL, day.GetYear(), day.GetMonth(), day.GetDay(), ::GetApp()->GetConfig(Elspot::EXCHANGERATESAPI_TOKEN_PROPERTY)));
    }
    else
    {
      uri = Poco::URI(fmt::sprintf(EUR_LATEST_URL, ::GetApp()->GetConfig(Elspot::EXCHANGERATESAPI_TOKEN_PROPERTY)));
    }
    std::unique_ptr<Poco::Net::HTTPClientSession> session = std::make_unique<Poco::Net::HTTPClientSession>(uri.getHost(), uri.getPort());

    // prepare path
    std::string path(uri.getPathAndQuery());
    if (path.empty())
      path = "/";

    // send request
    Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, path, Poco::Net::HTTPMessage::HTTP_1_1);
    session->setKeepAliveTimeout(Poco::Timespan(30, 0));
    session->sendRequest(req);

    // get response
    Poco::Net::HTTPResponse res;
    if (Poco::Net::HTTPResponse::HTTP_OK != res.getStatus())
    {
      return RegisterFail(day);
    }

    Poco::JSON::Parser parser;
    auto json_root = parser.parse(session->receiveResponse(res));

    if (!json_root)
    {
      return RegisterFail(day);
    }

    auto object_root = json_root.extract<Poco::JSON::Object::Ptr>();
    if (!object_root)
    {
      return RegisterFail(day);
    }

    std::string base_currency = object_root->getValue<std::string>("base");
    if (0 != base_currency.compare("EUR"))
    {
      return RegisterFail(day);
    }

    Poco::JSON::Object::Ptr rates_object = object_root->getObject("rates");
    if (!rates_object)
    {
      return RegisterFail(day);
    }

    //Remove any old values
    std::erase_if(m_rates, [day](const auto& item)
      {
        auto const& [key, value] = item;
        return key < (day.AsULong()-1);
      });

    double exchange_rate = rates_object->getValue<double>("NOK");
    m_rates.insert({day.AsULong(), exchange_rate});
#else
    Poco::Logger::get(Logger::DEFAULT).information("Currency::FetchEur hardcoding 10.2");
    m_rates.insert({day.AsULong(), 10.2});
#endif
    return true;
  }
  catch (Poco::Exception& ex)
  {
    Poco::Logger::get(Logger::DEFAULT).error(ex.message());
    return RegisterFail(day);
  }
  catch (...)
  {
    Poco::Logger::get(Logger::DEFAULT).error("Got currency exception");
    return RegisterFail(day);
  }
}

bool Currency::RegisterFail(const LocalDay& day)
{
  { //Lock scope
    const std::lock_guard<std::mutex> lock(m_failmap_mutex);

    Poco::Logger::get(Logger::DEFAULT).error(std::string("Fetching exchange rate failed for ")+day.ToString());
    m_failmap.insert({day.AsULong(), std::chrono::system_clock::now()});
  }
  
  return false;
}
