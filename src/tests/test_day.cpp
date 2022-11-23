#include "gtest/gtest.h"

#include "../day.h"


TEST(TestDay, EpochTest) {
  UTCTime utc_time(0L);
  EXPECT_EQ(utc_time.GetYear(), 1970);
  EXPECT_EQ(utc_time.GetMonth(), 1);
  EXPECT_EQ(utc_time.GetDay(), 1);
  EXPECT_EQ(utc_time.GetHour(), 0);
  EXPECT_EQ(utc_time.GetMinute(), 0);
  EXPECT_EQ(utc_time.GetSecond(), 0);
  EXPECT_EQ(utc_time.GetNorwegianTimezoneOffset(), 3600);
}

TEST(TestDay, SpringNoDSTTest) {
  UTCTime utc_time(1648342799L);
  EXPECT_EQ(utc_time.GetYear(), 2022);
  EXPECT_EQ(utc_time.GetMonth(), 3);
  EXPECT_EQ(utc_time.GetDay(), 27);
  EXPECT_EQ(utc_time.GetHour(), 0);
  EXPECT_EQ(utc_time.GetMinute(), 59);
  EXPECT_EQ(utc_time.GetSecond(), 59);
  EXPECT_EQ(utc_time.GetNorwegianTimezoneOffset(), 3600);
}

TEST(TestDay, SpringDSTTest) {
  UTCTime utc_time(1648342800L);
  EXPECT_EQ(utc_time.GetYear(), 2022);
  EXPECT_EQ(utc_time.GetMonth(), 3);
  EXPECT_EQ(utc_time.GetDay(), 27);
  EXPECT_EQ(utc_time.GetHour(), 1);
  EXPECT_EQ(utc_time.GetMinute(), 0);
  EXPECT_EQ(utc_time.GetSecond(), 0);
  EXPECT_EQ(utc_time.GetNorwegianTimezoneOffset(), 7200);
}

TEST(TestDay, AutumnDSTTest) {
  UTCTime utc_time(1667091599L);
  EXPECT_EQ(utc_time.GetYear(), 2022);
  EXPECT_EQ(utc_time.GetMonth(), 10);
  EXPECT_EQ(utc_time.GetDay(), 30);
  EXPECT_EQ(utc_time.GetHour(), 0);
  EXPECT_EQ(utc_time.GetMinute(), 59);
  EXPECT_EQ(utc_time.GetSecond(), 59);
  EXPECT_EQ(utc_time.GetNorwegianTimezoneOffset(), 7200);
}

TEST(TestDay, AutumnNoDSTTest) {
  UTCTime utc_time(1667091600L);
  EXPECT_EQ(utc_time.GetYear(), 2022);
  EXPECT_EQ(utc_time.GetMonth(), 10);
  EXPECT_EQ(utc_time.GetDay(), 30);
  EXPECT_EQ(utc_time.GetHour(), 1);
  EXPECT_EQ(utc_time.GetMinute(), 0);
  EXPECT_EQ(utc_time.GetSecond(), 0);
  EXPECT_EQ(utc_time.GetNorwegianTimezoneOffset(), 3600);
}

TEST(TestDay, Xmas2022Test) {
  UTCTime utc_time(1672444800L);
  EXPECT_EQ(utc_time.AsNorwegianDay().ToString(), "31.desember 2022");

  utc_time.SetTime(12, 15, 50);
  EXPECT_EQ(utc_time.AsNorwegianTime().ToString(), "13:15.50 31.desember 2022");

  utc_time.SetHour(22);
  utc_time.SetMinute(59);
  EXPECT_EQ(utc_time.AsNorwegianTime().ToString(), "23:59.50 31.desember 2022");

  utc_time = utc_time.IncrementSecondsCopy(15);
  EXPECT_EQ(utc_time.AsNorwegianTime().ToString(), "00:00.05 1.januar 2023");

  utc_time = utc_time.DecrementSecondsCopy(10);
  EXPECT_EQ(utc_time.AsNorwegianTime().ToString(), "23:59.55 31.desember 2022");
}

TEST(TestDay, UTCTimeEqualityTest) {
  UTCTime utc_time1(1672444800L);
  UTCTime utc_time2 = utc_time1.IncrementSecondsCopy(100);
  UTCTime utc_time3 = utc_time1.IncrementSecondsCopy(100);

  EXPECT_EQ(utc_time1<utc_time2, true);
  EXPECT_EQ(utc_time2<utc_time3, false);

  EXPECT_EQ(utc_time1==utc_time2, false);
  EXPECT_EQ(utc_time2==utc_time3, true);

  EXPECT_EQ(utc_time1>=utc_time2, false);
  EXPECT_EQ(utc_time2>=utc_time3, true);

  EXPECT_EQ(utc_time1!=utc_time2, true);
  EXPECT_EQ(utc_time2!=utc_time3, false);
}

TEST(TestDay, NorwegianDayTest) {
  UTCTime utc_time(1672444800L);
  NorwegianDay norwegian_day = utc_time.AsNorwegianDay();
  EXPECT_EQ(norwegian_day.AsULong(), 20221231);
  EXPECT_EQ(norwegian_day.GetYear(), 2022);
  EXPECT_EQ(norwegian_day.GetMonth(), 12);
  EXPECT_EQ(norwegian_day.GetDay(), 31);
}

TEST(TestDay, TodayTomorrowTest) {
  UTCTime now;
  EXPECT_EQ(now.AsNorwegianDay().IsToday(), true);

  UTCTime tomorrow = now.IncrementNorwegianDaysCopy(1);
  EXPECT_EQ(tomorrow.AsNorwegianDay().IsTomorrow(), true);
}

TEST(TestDay, DaysAfterTest) {
  UTCTime now;
  UTCTime four_weeks_ago = now.DecrementNorwegianDaysCopy(14);
  UTCTime in_four_weeks = now.IncrementNorwegianDaysCopy(14);
  EXPECT_EQ(now.AsNorwegianDay().DaysAfter(four_weeks_ago.AsNorwegianDay().AsULong()), 14);
  EXPECT_EQ(now.AsNorwegianDay().DaysAfter(four_weeks_ago.AsNorwegianDay()), 14);
  EXPECT_EQ(now.AsNorwegianDay().DaysAfter(in_four_weeks.AsNorwegianDay().AsULong()), -14);
  EXPECT_EQ(now.AsNorwegianDay().DaysAfter(in_four_weeks.AsNorwegianDay()), -14);
}


/*
  [[nodiscard]] signed long DaysAfter(unsigned long other) const {return DaysAfter(NorwegianDay(other));}
  [[nodiscard]] signed long DaysAfter(const NorwegianDay& other) const;

class NorwegianTime : public NorwegianDay
{
friend class UTCTime;
private:
  NorwegianTime(const struct tm time_tm_norwegiantime);
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

 */
