#ifndef _DAY_H_
#define _DAY_H_

#include <array>
#include <cstdint>
#include <string>
#include <time.h>


class LocalDay;
class LocalTime;
class UTCTime
{
public:
  UTCTime();
  UTCTime(const time_t& time);
private:
  void Initialize(const time_t& time);
public:
  [[nodiscard]] bool operator<(const UTCTime& other) const {return AsUTCTimeT()<other.AsUTCTimeT();}
  [[nodiscard]] bool operator==(const UTCTime& other) const {return AsUTCTimeT()==other.AsUTCTimeT();}
  [[nodiscard]] bool operator>=(const UTCTime& other) const {return AsUTCTimeT()>=other.AsUTCTimeT();}
  [[nodiscard]] bool operator!=(const UTCTime& other) const {return AsUTCTimeT()!=other.AsUTCTimeT();}
  [[nodiscard]] UTCTime Increment(const time_t& seconds);
  [[nodiscard]] UTCTime Decrement(const time_t& seconds) {return Increment(-seconds);}
  [[nodiscard]] const LocalDay AsLocalDay() const;
  [[nodiscard]] const LocalTime AsLocalTime() const;
  [[nodiscard]] time_t AsUTCTimeT() const {return m_time_utc;}
  [[nodiscard]] uint16_t GetYear() const {return m_time_tm_utc.tm_year+1900;}
  [[nodiscard]] uint8_t GetMonth() const {return m_time_tm_utc.tm_mon+1;}
  [[nodiscard]] uint8_t GetDay() const {return m_time_tm_utc.tm_mday;}
  [[nodiscard]] uint8_t GetHour() const {return m_time_tm_utc.tm_hour;}
  [[nodiscard]] uint8_t GetMinute() const {return m_time_tm_utc.tm_min;}
  [[nodiscard]] uint8_t GetSecond() const {return m_time_tm_utc.tm_sec;}
  [[nodiscard]] time_t GetLocalTimezoneOffset();

  void SetTime(uint8_t hour, uint8_t minute, uint8_t second);
  void SetHour(uint8_t hour);
  void SetMinute(uint8_t minute);
  void SetSecond(uint8_t second);
  
private:
  time_t m_time_utc;
  struct tm m_time_tm_utc;
};

class LocalDay
{
friend class UTCTime;
friend class LocalTime;
private:
  static constexpr std::array m_months{"januar", "februar", "mars", "april", "mai", "juni", "juli", "august", "september", "oktober", "november", "desember"};
private:
  LocalDay(const struct tm time_tm_localtime);
public:
  [[nodiscard]] bool operator<(const LocalDay& other) const {return AsULong()<other.AsULong();}
  [[nodiscard]] bool operator==(const LocalDay& other) const {return AsULong()==other.AsULong();}
  [[nodiscard]] bool operator!=(const LocalDay& other) const {return AsULong()!=other.AsULong();}
  [[nodiscard]] unsigned long AsULong() const {return (m_year%9999)*10*10*10*10 + (m_month%99)*10*10 + (m_day%99);}
  [[nodiscard]] virtual std::string ToString() const;
  [[nodiscard]] bool IsToday() const;
  [[nodiscard]] bool IsTomorrow() const;
  [[nodiscard]] uint16_t GetYear() const {return m_year;}
  [[nodiscard]] uint8_t GetMonth() const {return m_month;}
  [[nodiscard]] uint8_t GetDay() const {return m_day;}
private:
  uint16_t m_year;
  uint8_t  m_month;
  uint8_t  m_day;
};

class LocalTime : public LocalDay
{
friend class UTCTime;
private:
  LocalTime(const struct tm time_tm_localtime);
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
