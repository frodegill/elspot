#ifndef _MQTT_H_
#define _MQTT_H_

#include <string>
#include <mqtt/client.h>

#include "day.h"
#include "spotprice.h"


#if 0
(<sone> is "NO-1", "NO-2", "NO-3", "NO-4" or "NO-5")

/nordpool/today/exchangerate               : Exchangerate used for EUR-NOK. Set to NOK for 1EUR
/nordpool/today/<sone>/nok                 : Current price in NOK. Set to NOK/KWh
/nordpool/today/<sone>/eur                 : Current price in EUR. Set to EUR/KWh
/nordpool/today/<sone>/order               : Current order, from most expensive (0) to least expensive (23). Set to 0-23
/nordpool/today/<sone>/nok<[00]-[23]>      : Price in NOK for a given hour. Set to NOK/KWh
/nordpool/today/<sone>/eur<[00]-[23]>      : Price in EUR for a given hour. Set to EUR/KWh
/nordpool/today/<sone>/order[00]-[23]>     : Order for a given hour, from most expensive (0) to least expensive (23). Set to 0-23
/nordpool/today/<sone>/sorted<[0]-[23]>    : Hour reference from the most expensive (0) to least expensive (23). Set to 00-23
/nordpool/tomorrow/exchangerate            : Exchangerate used for EUR-NOK. Set to NOK for 1EUR
/nordpool/tomorrow/<sone>/nok<[00]-[23]>   : Price in NOK for a given hour. Set to NOK/KWh
/nordpool/tomorrow/<sone>/eur<[00]-[23]>   : Price in EUR for a given hour. Set to EUR/KWh
/nordpool/tomorrow/<sone>/order[00]-[23]>  : Order for a given hour, from most expensive (0) to least expensive (23). Set to 0-23
/nordpool/tomorrow/<sone>/sorted<[0]-[23]> : Hour reference from the most expensive (0) to least expensive (23). Set to 00-23
#endif

struct Price
{
  int hour;
  double price;
};


class MQTT : public virtual mqtt::callback
{
public:
  static constexpr const char* CLIENT_ID = "elspot";
  static constexpr int MAX_BUFFERED_MESSAGES = 4 + 4*24 + 1 + 4*24; //One message pr MQTT topic (as documented above)

public:
  MQTT();

public:
  void connection_lost(const std::string& cause) override;
  
public:
  [[nodiscard]] bool GotPrices(const LocalDay& day);
  [[nodiscard]] bool PublishCurrentPrices();

private:
  [[nodiscard]] bool Publish(const std::string& topic, const double& value, int precision=2);
  [[nodiscard]] bool Publish(const std::string& topic, const std::string& value);
  [[nodiscard]] bool GetInfo(const LocalDay& day, Spotprice::AreaRateType& area_rates, double& exchange_rate) const;
  void CopyAndSortRates(const Spotprice::DayRateType& eur_rates, std::array<Price,Spotprice::HOURS_PER_DAY>& sorted_prices) const;
  
public:
  [[nodiscard]] static std::string DoubleToString(const double& value, int precision);
  
private:
  std::unique_ptr<mqtt::client> m_mqtt_client;
  mqtt::connect_options m_connection_options;

  std::mutex m_connection_mutex;
};

#endif // _MQTT_H_
