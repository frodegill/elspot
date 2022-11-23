#ifndef _DAY_H_
#define _DAY_H_

#include <array>
#include <cstdint>
#include <ctime>
#include <string>
#include <time.h>


class NorwegianDay;
class NorwegianTime;
class UTCTime
{
public:
  UTCTime();
  UTCTime(const std::time_t& time);
private:
  void Initialize(const std::time_t& time);
public:
  [[nodiscard]] bool operator<(const UTCTime& other) const {return AsUTCTimeT()<other.AsUTCTimeT();}
  [[nodiscard]] bool operator==(const UTCTime& other) const {return AsUTCTimeT()==other.AsUTCTimeT();}
  [[nodiscard]] bool operator>=(const UTCTime& other) const {return AsUTCTimeT()>=other.AsUTCTimeT();}
  [[nodiscard]] bool operator!=(const UTCTime& other) const {return AsUTCTimeT()!=other.AsUTCTimeT();}
  [[nodiscard]] UTCTime IncrementSecondsCopy(const std::time_t& seconds) const;
  [[nodiscard]] UTCTime IncrementHoursCopy(const std::time_t& hours) const {return IncrementSecondsCopy(hours*60*60);}
  [[nodiscard]] UTCTime IncrementNorwegianDaysCopy(const std::time_t& days) const;
  [[nodiscard]] UTCTime DecrementSecondsCopy(const std::time_t& seconds) const {return IncrementSecondsCopy(-seconds);}
  [[nodiscard]] UTCTime DecrementHoursCopy(const std::time_t& hours) const {return IncrementHoursCopy(-hours);}
  [[nodiscard]] UTCTime DecrementNorwegianDaysCopy(const std::time_t& days) const {return IncrementNorwegianDaysCopy(-days);}
  [[nodiscard]] const NorwegianDay AsNorwegianDay() const;
  [[nodiscard]] const NorwegianTime AsNorwegianTime() const;
  [[nodiscard]] std::time_t AsUTCTimeT() const {return m_time_utc;}
  [[nodiscard]] uint16_t GetYear() const {return static_cast<uint16_t>(m_time_tm_utc.tm_year)+1900;}
  [[nodiscard]] uint8_t GetMonth() const {return static_cast<uint8_t>(m_time_tm_utc.tm_mon)+1;}
  [[nodiscard]] uint8_t GetDay() const {return static_cast<uint8_t>(m_time_tm_utc.tm_mday);}
  [[nodiscard]] uint8_t GetHour() const {return static_cast<uint8_t>(m_time_tm_utc.tm_hour);}
  [[nodiscard]] uint8_t GetMinute() const {return static_cast<uint8_t>(m_time_tm_utc.tm_min);}
  [[nodiscard]] uint8_t GetSecond() const {return static_cast<uint8_t>(m_time_tm_utc.tm_sec);}
  [[nodiscard]] UTCTime DaylightSavingStart() const;
  [[nodiscard]] UTCTime DaylightSavingEnd() const;
  [[nodiscard]] std::time_t GetNorwegianTimezoneOffset() const;

  void SetTime(uint8_t hour, uint8_t minute, uint8_t second);
  void SetHour(uint8_t hour);
  void SetMinute(uint8_t minute);
  void SetSecond(uint8_t second);
  
private:
  [[nodiscard]] uint8_t LastSundayInMonth(uint16_t year, uint8_t month) const;

private:
  std::time_t m_time_utc;
  std::tm m_time_tm_utc;
};

class NorwegianDay
{
friend class UTCTime;
friend class NorwegianTime;
private:
  static constexpr std::array<const char*, 12> m_months{"januar", "februar", "mars", "april", "mai", "juni", "juli", "august", "september", "oktober", "november", "desember"};
private:
  NorwegianDay(const std::tm time_tm_norwegiantime);
  NorwegianDay(unsigned long as_ulong);
  NorwegianDay(uint16_t year, uint8_t month, uint8_t day) : m_year(year), m_month(month), m_day(day) {}
public:
  [[nodiscard]] bool operator<(const NorwegianDay& other) const {return AsULong()<other.AsULong();}
  [[nodiscard]] bool operator==(const NorwegianDay& other) const {return AsULong()==other.AsULong();}
  [[nodiscard]] bool operator!=(const NorwegianDay& other) const {return AsULong()!=other.AsULong();}
  [[nodiscard]] unsigned long AsULong() const {return (m_year%9999)*10*10*10*10 + (m_month%99)*10*10 + (m_day%99);}
  [[nodiscard]] virtual std::string ToString() const;
  [[nodiscard]] bool IsToday() const;
  [[nodiscard]] bool IsTomorrow() const;
  [[nodiscard]] signed long DaysAfter(unsigned long other) const {return DaysAfter(NorwegianDay(other));}
  [[nodiscard]] signed long DaysAfter(const NorwegianDay& other) const;
  [[nodiscard]] uint16_t GetYear() const {return m_year;}
  [[nodiscard]] uint8_t GetMonth() const {return m_month;}
  [[nodiscard]] uint8_t GetDay() const {return m_day;}
private:
  uint16_t m_year;
  uint8_t  m_month;
  uint8_t  m_day;
};

class NorwegianTime : public NorwegianDay
{
friend class UTCTime;
private:
  NorwegianTime(const std::tm time_tm_norwegiantime);
public:
  [[nodiscard]] uint8_t GetHour() const {return m_hour;}
  [[nodiscard]] uint8_t GetMinute() const {return m_minute;}
  [[nodiscard]] uint8_t GetSecond() const {return m_second;}
  [[nodiscard]] std::string ToString() const override;
private:
  uint8_t  m_hour;
  uint8_t  m_minute;
  uint8_t  m_second;
};

#endif // _MQTT_H_
