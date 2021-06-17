#include "spotprice_cron.h"

#include "application.h"
#include "day.h"


void SpotpriceCron::main()
{
  LocalDay most_recent_today = UTCTime(0).AsLocalDay();
  LocalDay most_recent_tomorrow = UTCTime(0).AsLocalDay();

  while(true)
  {
    UTCTime now;
    LocalDay today = now.AsLocalDay();
    LocalDay tomorrow = now.Increment(24*60*60).AsLocalDay();

    //If we have not cached today, cache today immediately!!!
    if (most_recent_today != today)
    {
      if (::GetApp()->getSpotprice()->CacheEurRates(today) &&
          ::GetApp()->getMQTT()->GotPrices(today) &&
          ::GetApp()->getSVG()->GenerateSVGs(today))
      {
        most_recent_today = today;
      }
      else
      {
        Poco::Logger::get(Logger::DEFAULT).error("Caching spotprice for today failed");
      }
    }

    //According to Nordpool, final day-ahead bids must be submitted before 1200 CET, and "typically announced to the market at 12:42 CET or later". 1242 CET is 1142 UTC.
    //To not jump the gun too early, try polling every 20 minute starting at 1400 UTC

    //Wait until at least 1400 UTC (unless caching of today failed. In that case, caching of today should be retried after a short pause)
    UTCTime poll_time;
    poll_time = poll_time.Increment(poll_time.GetLocalTimezoneOffset()); //Make sure we're at the correct local_day
    poll_time.SetTime(14, 0, 0);

    if (most_recent_today==today && most_recent_tomorrow!=tomorrow && now<poll_time)
    {
      std::this_thread::sleep_until(std::chrono::system_clock::from_time_t(poll_time.AsUTCTimeT()));
    }

    //After 1400 UTC, try to cache any not already cached prices up to 3 times an hour (at XX:00, XX:20 and XX:40)
    now = UTCTime();
    if (now >= poll_time)
    {
      if (most_recent_today!=today || most_recent_tomorrow!=tomorrow)
      {
        poll_time = UTCTime().Increment(20*60);
        poll_time.SetMinute((poll_time.GetMinute()/20)*20); //Integer division to round down to 00|20|40
        poll_time.SetSecond(0);
        std::this_thread::sleep_until(std::chrono::system_clock::from_time_t(poll_time.AsUTCTimeT()));

        //If, however unlikely, we have waited until midnight, restart loop iteration to reflect change of day
        if (UTCTime().AsLocalDay() != today)
        {
          Poco::Logger::get(Logger::DEFAULT).information("SpotpriceCron passed midnight");
          continue;
        }
        
        //Are we STILL not having todays prices? Keep on trying..
        if (most_recent_today != today)
        {
          if (::GetApp()->getSpotprice()->CacheEurRates(today) &&
              ::GetApp()->getMQTT()->GotPrices(today) &&
              ::GetApp()->getSVG()->GenerateSVGs(today))
          {
            most_recent_today = today;
          }
          else
          {
            Poco::Logger::get(Logger::DEFAULT).error("Caching spotprice for today failed again");
          }
        }
        
        //And chech if Nordpool has published prices for tomorrow
        now = UTCTime();
        if (most_recent_tomorrow != tomorrow)
        {
          if (::GetApp()->getSpotprice()->CacheEurRates(tomorrow) &&
              ::GetApp()->getMQTT()->GotPrices(tomorrow) &&
              ::GetApp()->getSVG()->GenerateSVGs(tomorrow))
          {
            most_recent_tomorrow = tomorrow;
          }
          else
          {
            Poco::Logger::get(Logger::DEFAULT).error("Caching spotprice for tomorrow failed at " + now.AsLocalTime().ToString());
          }
        }
      }
      else //Eveything is done for today. Wait until midnight local time (should even work for days with 23 or 25 hours)
      {
        UTCTime now;
        LocalTime midnight_localtime = now.AsLocalTime();
        UTCTime midnight_utc = now.Increment((24-midnight_localtime.GetHour())*60*60);
        midnight_utc.SetMinute(0);
        midnight_utc.SetSecond(0);

        std::this_thread::sleep_until(std::chrono::system_clock::from_time_t(midnight_utc.AsUTCTimeT()));
      }
    }
  }
}
