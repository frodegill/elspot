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
  static constexpr int QUARTERS_PER_DAY = HOURS_PER_DAY*4;
  static constexpr std::array<Area,5> m_areas
    {{
      {"NO-1", "10YNO-1--------2", "Oslo"},
      {"NO-2", "10YNO-2--------T", "Kristiansand"},
      {"NO-3", "10YNO-3--------J", "Trondheim"},
      {"NO-4", "10YNO-4--------9", "Tromsø"},
      {"NO-5", "10Y1001A1001A48H", "Bergen"}
    }};
    typedef std::array<double,QUARTERS_PER_DAY> QuarterRateType;
    typedef std::array<QuarterRateType,m_areas.size()> AreaQuarterRateType;

public:
  [[nodiscard]] virtual bool HasEurRate(const NorwegianDay& norwegian_day) const;
  [[nodiscard]] virtual bool CacheEurRates(const NorwegianDay& norwegian_day);
  [[nodiscard]] virtual bool GetEurQuarterRates(const NorwegianDay& norwegian_day, AreaQuarterRateType& eur_quarter_rates);

  [[nodiscard]] static double GetHourRate(const QuarterRateType& eur_rates, unsigned int hour);
  [[nodiscard]] static double GetQuarterRate(const QuarterRateType& eur_rates, unsigned int quarter);

private:
  [[nodiscard]] virtual bool FetchEurQuarterRates(const NorwegianDay& norwegian_day); //Not thread-safe funtion. Call from within locked m_eur_quarter_rates_mutex
  [[nodiscard]] virtual bool RegisterFail(const NorwegianDay& norwegian_day);

private:
  std::map<unsigned long, AreaQuarterRateType> m_eur_quarter_rates;
  mutable std::mutex m_eur_quarter_rates_mutex;

  std::map<unsigned long, std::chrono::system_clock::time_point> m_failmap;
  std::mutex m_failmap_mutex;
};

#endif // _SPOTPRICE_H_
