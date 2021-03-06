#include "day.h"

#include <chrono>
#include <cstring>

#include <fmt/printf.h>


UTCTime::UTCTime()
{
  Initialize(std::chrono::system_clock::to_time_t(std::chrono::system_clock::now()));
}

UTCTime::UTCTime(const time_t& time)
{
  Initialize(time);
}

void UTCTime::Initialize(const time_t& time)
{
  m_time_utc = time;
  gmtime_r(&m_time_utc, &m_time_tm_utc);
}

UTCTime UTCTime::Increment(const time_t& seconds)
{
  return UTCTime(m_time_utc + seconds);
}

const LocalDay UTCTime::AsLocalDay() const
{
  struct tm time_tm_localtime;
  localtime_r(&m_time_utc, &time_tm_localtime);
  return LocalDay(time_tm_localtime);
}

const LocalTime UTCTime::AsLocalTime() const
{
  struct tm time_tm_localtime;
  localtime_r(&m_time_utc, &time_tm_localtime);
  return LocalTime(time_tm_localtime);
}

time_t UTCTime::GetLocalTimezoneOffset()
{
  struct tm time_tm_localtime;
  localtime_r(&m_time_utc, &time_tm_localtime);
  return time_tm_localtime.tm_gmtoff;
}

void UTCTime::SetTime(uint8_t hour, uint8_t minute, uint8_t second)
{
  m_time_utc += (hour - m_time_tm_utc.tm_hour)*60*60;
  m_time_utc += (minute - m_time_tm_utc.tm_min)*60;
  m_time_utc += (second - m_time_tm_utc.tm_sec);
  gmtime_r(&m_time_utc, &m_time_tm_utc);
}

void UTCTime::SetHour(uint8_t hour)
{
  m_time_utc += (hour - m_time_tm_utc.tm_hour)*60*60;
  gmtime_r(&m_time_utc, &m_time_tm_utc);
}

void UTCTime::SetMinute(uint8_t minute)
{
  m_time_utc += (minute - m_time_tm_utc.tm_min)*60;
  gmtime_r(&m_time_utc, &m_time_tm_utc);
}

void UTCTime::SetSecond(uint8_t second)
{
  m_time_utc += (second - m_time_tm_utc.tm_sec);
  gmtime_r(&m_time_utc, &m_time_tm_utc);
}


LocalDay::LocalDay(const struct tm time_tm_localtime) :
  m_year(time_tm_localtime.tm_year+1900),
  m_month(time_tm_localtime.tm_mon+1),
  m_day(time_tm_localtime.tm_mday)
{
}

LocalDay::LocalDay(unsigned long as_ulong)
{
  m_day = as_ulong%100;
  as_ulong = (as_ulong - m_day) / 100;
  m_month = as_ulong%100;
  as_ulong = (as_ulong - m_month) / 100;
  m_year = as_ulong;
}

std::string LocalDay::ToString() const
{
  return fmt::sprintf("%u.%s %u", GetDay()%99, m_months[GetMonth()-1], GetYear()%9999);
}

bool LocalDay::IsToday() const
{
  LocalDay today = UTCTime().AsLocalDay();
  return AsULong() == today.AsULong();
}

bool LocalDay::IsTomorrow() const
{
  UTCTime today = UTCTime();
  LocalDay tomorrow = today.Increment(24*60*60).AsLocalDay();
  return AsULong() == tomorrow.AsULong();
}

signed long LocalDay::DaysAfter(const LocalDay& other) const
{
  std::tm t;
  t.tm_year = GetYear() - 1900;
  t.tm_mon = GetMonth() - 1;
  t.tm_mday = GetDay();
  t.tm_hour = 12;
  t.tm_min = t.tm_sec = 0;
  std::time_t tt = std::mktime(&t);
  if (-1 == tt)
  {
    return -9999;
  }

  std::tm o;
  o.tm_year = other.GetYear() - 1900;
  o.tm_mon = other.GetMonth() - 1;
  o.tm_mday = other.GetDay();
  o.tm_hour = 12;
  o.tm_min = o.tm_sec = 0;
  std::time_t to = std::mktime(&o);
  if (-1 == to)
  {
    return 9999;
  }

  return std::difftime(tt, to) / (60 * 60 * 24);
}


LocalTime::LocalTime(const struct tm time_tm_localtime) :
  LocalDay(time_tm_localtime),
  m_hour(time_tm_localtime.tm_hour),
  m_minute(time_tm_localtime.tm_min),
  m_second(time_tm_localtime.tm_sec)
{
}

std::string LocalTime::ToString() const
{
  return fmt::sprintf("%02u:%02u.%02u %u.%s %u", GetHour()%99, GetMinute()%99, GetSecond()%99, GetDay()%99, m_months[GetMonth()-1], GetYear()%9999);
}
