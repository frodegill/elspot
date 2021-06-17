#ifndef _CURRENCY_H_
#define _CURRENCY_H_

#include <chrono>
#include <map>
#include <mutex>

#include "day.h"


class Currency
{
private:
  static constexpr std::chrono::minutes RETRY_DURATION = std::chrono::minutes(10);
  static constexpr const char* EUR_HISTORICAL_URL = "http://api.exchangeratesapi.io/v1/%04d-%02d-%02d?access_key=%s&base=EUR&symbols=NOK";
  static constexpr const char* EUR_LATEST_URL   = "http://api.exchangeratesapi.io/v1/latest?access_key=%s&base=EUR&symbols=NOK";

public:
  [[nodiscard]] bool GetCurrentExchangeRate(double& rate);
  [[nodiscard]] bool GetExchangeRate(const LocalDay& day, double& rate);

private:
  [[nodiscard]] bool FetchEur(const LocalDay& day); //Not thread-safe funtion. Call from within locked m_rates_mutex
  [[nodiscard]] bool RegisterFail(const LocalDay& day);

private:
  std::map<unsigned long, double> m_rates;
  std::mutex m_rates_mutex;
  
  std::map<unsigned long, std::chrono::system_clock::time_point> m_failmap;
  std::mutex m_failmap_mutex;
};

#endif // _CURRENCY_H_
