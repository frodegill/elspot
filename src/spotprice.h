#ifndef _SPOTPRICE_H_
#define _SPOTPRICE_H_

#include <array>
#include <chrono>
#include <map>
#include <mutex>

#include <Poco/DOM/Node.h>

#include "day.h"


struct Area
{
  const char* id;
  const char* code;
  const char* name;
};


class Spotprice
{
public:
  virtual ~Spotprice() = default;

private:
  static constexpr std::chrono::minutes RETRY_DURATION = std::chrono::minutes(10);
  static constexpr const char* DAYAHEAD_URL = "https://web-api.tp.entsoe.eu/api?securityToken=%s&documentType=A44&in_Domain=%s&out_Domain=%s&periodStart=%08lu1200&periodEnd=%08lu1300"; //Ask for only one hour mid-day. We will get entire day

public:
  static constexpr int HOURS_PER_DAY = 24;
  static constexpr std::array<Area,5> m_areas
    {{
      {"NO-1", "10YNO-1--------2", "Oslo"},
      {"NO-2", "10YNO-2--------T", "Kristiansand"},
      {"NO-3", "10YNO-3--------J", "Trondheim"},
      {"NO-4", "10YNO-4--------9", "Tromsø"},
      {"NO-5", "10Y1001A1001A48H", "Bergen"}
    }};
    typedef std::array<double,HOURS_PER_DAY> DayRateType;
    typedef std::array<DayRateType,m_areas.size()> AreaRateType;

public:
  [[nodiscard]] virtual bool HasEurRate(const NorwegianDay& norwegian_day) const;
  [[nodiscard]] virtual bool CacheEurRates(const NorwegianDay& norwegian_day);
  [[nodiscard]] virtual bool GetEurRates(const NorwegianDay& norwegian_day, AreaRateType& eur_rates);
  
private:
  [[nodiscard]] virtual bool FetchEurRates(const NorwegianDay& norwegian_day); //Not thread-safe funtion. Call from within locked m_eur_rates_mutex
  [[nodiscard]] virtual bool RegisterFail(const NorwegianDay& norwegian_day);

private:
  std::map<unsigned long, AreaRateType> m_eur_rates;
  mutable std::mutex m_eur_rates_mutex;

  std::map<unsigned long, std::chrono::system_clock::time_point> m_failmap;
  std::mutex m_failmap_mutex;
};

#endif // _SPOTPRICE_H_
