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

bool MQTT::GotPrices(const LocalDay& day)
{
  try
  { //Lock scope
    const std::lock_guard<std::mutex> lock(MQTT::m_connection_mutex);

    bool is_today = day.IsToday();
    if (!is_today && !day.IsTomorrow())
    {
      Poco::Logger::get(Logger::DEFAULT).error(day.ToString()+std::string(" is not today/tomorrow"));
      return false;
    }

    Spotprice::AreaRateType area_rates;
    double exchange_rate;
    if (!GetInfo(day, area_rates, exchange_rate))
    {
      Poco::Logger::get(Logger::DEFAULT).information("MQTT GotPrices failed at GetInfo");
      return false;
    }

    bool was_connected = m_mqtt_client->is_connected();
    if (!was_connected)
    {
      m_mqtt_client->connect(m_connection_options);
    }

    bool status = Publish(is_today ? "/nordpool/today/exchangerate" : "/nordpool/tomorrow/exchangerate", exchange_rate);

    Spotprice::DayRateType eur_rates;
    std::array<Price,Spotprice::HOURS_PER_DAY> sorted_prices;
    for (std::array<Area,5>::size_type area_index=0; area_index<area_rates.size(); area_index++)
    {
      eur_rates = area_rates[area_index];
      CopyAndSortRates(eur_rates, sorted_prices);
      
      for (int index=0; index<Spotprice::HOURS_PER_DAY; index++)
      {
        status &= Publish(fmt::sprintf(is_today ? "/nordpool/today/%s/nok%02d" : "/nordpool/tomorrow/%s/nok%02d", Spotprice::m_areas[area_index].id, index), eur_rates[index] * exchange_rate);
        status &= Publish(fmt::sprintf(is_today ? "/nordpool/today/%s/eur%02d" : "/nordpool/tomorrow/%s/eur%02d", Spotprice::m_areas[area_index].id, index), eur_rates[index]);
        status &= Publish(fmt::sprintf(is_today ? "/nordpool/today/%s/order%02d" : "/nordpool/tomorrow/%s/order%02d", Spotprice::m_areas[area_index].id, index),
                fmt::sprintf("%d", std::lower_bound(sorted_prices.begin(), sorted_prices.end(), eur_rates[index], [](const Price& a, double b) {return a.price > b;}) - sorted_prices.begin()));
        status &= Publish(fmt::sprintf(is_today ? "/nordpool/today/%s/sorted%d" : "/nordpool/tomorrow/%s/sorted%d", Spotprice::m_areas[area_index].id, index),
                fmt::sprintf("%02d", sorted_prices[index].hour));
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
    LocalTime local_now = UTCTime().AsLocalTime();
    Spotprice::AreaRateType area_rates;
    double exchange_rate;
    if (!GetInfo(local_now, area_rates, exchange_rate))
    {
      return false;
    }

    bool was_connected = m_mqtt_client->is_connected();
    if (!was_connected)
    {
      m_mqtt_client->connect(m_connection_options);
    }

    Spotprice::DayRateType eur_rates;
    std::array<Price,Spotprice::HOURS_PER_DAY> sorted_prices;
    bool status = true;
    for (std::array<Area,5>::size_type area_index=0; area_index<area_rates.size(); area_index++)
    {
      eur_rates = area_rates[area_index];
      CopyAndSortRates(eur_rates, sorted_prices);
      
      status &= Publish(fmt::sprintf("/nordpool/today/%s/nok", Spotprice::m_areas[area_index].id), eur_rates[local_now.GetHour()] * exchange_rate);
      status &= Publish(fmt::sprintf("/nordpool/today/%s/eur", Spotprice::m_areas[area_index].id), eur_rates[local_now.GetHour()]);
      status &= Publish(fmt::sprintf("/nordpool/today/%s/order", Spotprice::m_areas[area_index].id),
              fmt::sprintf("%d", std::lower_bound(sorted_prices.begin(), sorted_prices.end(), eur_rates[local_now.GetHour()], [](const Price& a, double b) {return a.price > b;}) - sorted_prices.begin()));
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

bool MQTT::GetInfo(const LocalDay& day, Spotprice::AreaRateType& area_rates, double& exchange_rate) const
{
  if (!::GetApp()->getSpotprice()->GetEurRates(day, area_rates))
  {
    Poco::Logger::get(Logger::DEFAULT).error(std::string("Failed to get EUR rates for ") + day.ToString());
    return false;
  }

  if (!::GetApp()->getCurrency()->GetExchangeRate(day, exchange_rate))
  {
    Poco::Logger::get(Logger::DEFAULT).error(std::string("Failed to get exchange rate for ") + day.ToString());
    return false;
  }
  
  return true;
}

void MQTT::CopyAndSortRates(const Spotprice::DayRateType& eur_rates, std::array<Price,Spotprice::HOURS_PER_DAY>& sorted_prices) const
{
  //Copy and sort
  for (int hour=0; hour<Spotprice::HOURS_PER_DAY; hour++)
  {
    sorted_prices[hour].hour = hour;
    sorted_prices[hour].price = eur_rates[hour];
  }
  std::sort(sorted_prices.begin(), sorted_prices.end(), [](const Price& a, const Price& b) {return a.price > b.price;});
}

std::string MQTT::DoubleToString(const double& value, int precision)
{
  std::ostringstream stream;
  stream << std::fixed;
  stream << std::setprecision(precision);
  stream << value;
  return stream.str();
}
