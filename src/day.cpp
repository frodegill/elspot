#include "day.h"

#include <chrono>
#include <cstring>

#include <fmt/printf.h>


UTCTime::UTCTime()
{
  Initialize(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
}

UTCTime::UTCTime(const std::time_t& time)
{
  Initialize(time);
}

void UTCTime::Initialize(const std::time_t& time)
{
  m_time_utc = time;
  ::gmtime_r(&m_time_utc, &m_time_tm_utc);
}

UTCTime UTCTime::IncrementSecondsCopy(const std::time_t& seconds) const
{
  return UTCTime(m_time_utc + seconds);
}

UTCTime UTCTime::IncrementNorwegianDaysCopy(const std::time_t& days) const
{
  UTCTime adjusted_time(m_time_utc + days*24*60*60);
  return adjusted_time.DecrementSecondsCopy(GetNorwegianTimezoneOffset() - adjusted_time.GetNorwegianTimezoneOffset());
}

const NorwegianDay UTCTime::AsNorwegianDay() const
{
  std::tm time_tm_norwegiantime;
  localtime_r(&m_time_utc, &time_tm_norwegiantime);
  return NorwegianDay(time_tm_norwegiantime);
}

const NorwegianTime UTCTime::AsNorwegianTime() const
{
  std::tm time_tm_norwegiantime;
  localtime_r(&m_time_utc, &time_tm_norwegiantime);
  return NorwegianTime(time_tm_norwegiantime);
}

UTCTime UTCTime::DaylightSavingStart() const
{
  std::tm dst_tm;
  std::memset(&dst_tm, 0, sizeof(std::tm));
  dst_tm.tm_year = m_time_tm_utc.tm_year;
  dst_tm.tm_mon = 3-1;
  dst_tm.tm_mday = LastSundayInMonth(static_cast<uint16_t>(dst_tm.tm_year+1900), static_cast<uint8_t>(dst_tm.tm_mon+1));
  dst_tm.tm_hour = 1;
  dst_tm.tm_min = 0;
  dst_tm.tm_sec = 0;
  return UTCTime(::timegm(&dst_tm));
}

UTCTime UTCTime::DaylightSavingEnd() const
{
  std::tm dst_tm;
  std::memset(&dst_tm, 0, sizeof(std::tm));
  dst_tm.tm_year = m_time_tm_utc.tm_year;
  dst_tm.tm_mon = 10-1;
  dst_tm.tm_mday = LastSundayInMonth(static_cast<uint16_t>(dst_tm.tm_year+1900), static_cast<uint8_t>(dst_tm.tm_mon+1));
  dst_tm.tm_hour = 1;
  dst_tm.tm_min = 0;
  dst_tm.tm_sec = 0;
  return UTCTime(::timegm(&dst_tm));
}

std::time_t UTCTime::GetNorwegianTimezoneOffset() const
{
  UTCTime dst_start = DaylightSavingStart();
  UTCTime dst_end = DaylightSavingEnd();
  return (*this<dst_start || *this>=dst_end) ? 3600 : 7200;
}

uint8_t UTCTime::LastSundayInMonth(uint16_t year, uint8_t month) const
{
  uint8_t wday;

  //From unix/time.c
  {
    constexpr short monthbegin[12] = {0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334};
    std::time_t t = monthbegin[month-1] // full months
              + (!(year & 3) && (month-1) > 1); // leap day this year
    t += 365 * (year - 1970) // full years
        + (year - 1969)/4; // past leap days
    wday = static_cast<uint8_t>(static_cast<int>(t + 3) % 7);
  }

  int first_of_month_wday = (wday % 7 + 7) % 7;
  return (first_of_month_wday<=3) ? static_cast<uint8_t>(28-first_of_month_wday) : static_cast<uint8_t>(35-first_of_month_wday);
}

void UTCTime::SetTime(uint8_t hour, uint8_t minute, uint8_t second)
{
  m_time_utc += (hour - m_time_tm_utc.tm_hour)*60*60;
  m_time_utc += (minute - m_time_tm_utc.tm_min)*60;
  m_time_utc += (second - m_time_tm_utc.tm_sec);
  ::gmtime_r(&m_time_utc, &m_time_tm_utc);
}

void UTCTime::SetHour(uint8_t hour)
{
  m_time_utc += (hour - m_time_tm_utc.tm_hour)*60*60;
  ::gmtime_r(&m_time_utc, &m_time_tm_utc);
}

void UTCTime::SetMinute(uint8_t minute)
{
  m_time_utc += (minute - m_time_tm_utc.tm_min)*60;
  ::gmtime_r(&m_time_utc, &m_time_tm_utc);
}

void UTCTime::SetSecond(uint8_t second)
{
  m_time_utc += (second - m_time_tm_utc.tm_sec);
  ::gmtime_r(&m_time_utc, &m_time_tm_utc);
}


NorwegianDay::NorwegianDay(const std::tm time_tm_norwegiantime) :
  m_year(static_cast<uint16_t>(time_tm_norwegiantime.tm_year)+1900),
  m_month(static_cast<uint8_t>(time_tm_norwegiantime.tm_mon)+1),
  m_day(static_cast<uint8_t>(time_tm_norwegiantime.tm_mday))
{
}

NorwegianDay::NorwegianDay(unsigned long as_ulong)
{
  m_day = static_cast<uint8_t>(as_ulong%100);
  as_ulong = (as_ulong - m_day) / 100;
  m_month = static_cast<uint8_t>(as_ulong%100);
  as_ulong = (as_ulong - m_month) / 100;
  m_year = static_cast<uint16_t>(as_ulong);
}

std::string NorwegianDay::ToString() const
{
  return fmt::sprintf("%u.%s %u", GetDay()%99, m_months[GetMonth()-1], GetYear()%9999);
}

bool NorwegianDay::IsToday() const
{
  NorwegianDay today = UTCTime().AsNorwegianDay();
  return AsULong() == today.AsULong();
}

bool NorwegianDay::IsTomorrow() const
{
  UTCTime today = UTCTime();
  NorwegianDay tomorrow = today.IncrementNorwegianDaysCopy(1).AsNorwegianDay();
  return AsULong() == tomorrow.AsULong();
}

signed long NorwegianDay::DaysAfter(const NorwegianDay& other) const
{
  std::tm t;
  std::memset(&t, 0, sizeof(std::tm));
  t.tm_year = GetYear() - 1900;
  t.tm_mon = GetMonth() - 1;
  t.tm_mday = GetDay();
  t.tm_hour = 12;
  t.tm_min = t.tm_sec = 0;
  std::time_t tt = ::timegm(&t);
  if (-1 == tt)
  {
    return -9999;
  }

  std::tm o;
  std::memset(&o, 0, sizeof(std::tm));
  o.tm_year = other.GetYear() - 1900;
  o.tm_mon = other.GetMonth() - 1;
  o.tm_mday = other.GetDay();
  o.tm_hour = 12;
  o.tm_min = o.tm_sec = 0;
  std::time_t to = ::timegm(&o);
  if (-1 == to)
  {
    return 9999;
  }

  return static_cast<signed long>(std::difftime(tt, to) / (60 * 60 * 24));
}


NorwegianTime::NorwegianTime(const std::tm time_tm_norwegiantime) :
  NorwegianDay(time_tm_norwegiantime),
  m_hour(static_cast<uint8_t>(time_tm_norwegiantime.tm_hour)),
  m_minute(static_cast<uint8_t>(time_tm_norwegiantime.tm_min)),
  m_second(static_cast<uint8_t>(time_tm_norwegiantime.tm_sec))
{
}

std::string NorwegianTime::ToString() const
{
  return fmt::sprintf("%02u:%02u.%02u %u.%s %u", GetHour()%99, GetMinute()%99, GetSecond()%99, GetDay()%99, m_months[GetMonth()-1], GetYear()%9999);
}
