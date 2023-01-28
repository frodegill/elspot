#include "spotprice_cron.h"

#include <condition_variable>

#include "application.h"
#include "day.h"


std::condition_variable_any spotprice_cv;
std::mutex spotprice_mutex;


void spotprice_cron(std::stop_token token)
{
  bool first = true;
  bool failed = false;
  NorwegianDay most_recent_norwegian_today = UTCTime(0).AsNorwegianDay();
  NorwegianDay most_recent_norwegian_tomorrow = UTCTime(0).AsNorwegianDay();

  while(!token.stop_requested())
  {
    if (failed)
    {
      Poco::Logger::get(Logger::DEFAULT).warning("Waiting 5 minutes because of previous fail");
      std::unique_lock<std::mutex> lock(spotprice_mutex);
      if (spotprice_cv.wait_for(lock, token, std::chrono::minutes(5), [token]{return token.stop_requested();}))
      {
        Poco::Logger::get(Logger::DEFAULT).warning("Stop requested");
        return;
      }
      failed = false;
    }

    UTCTime now;
    NorwegianDay norwegian_today = now.AsNorwegianDay();
    NorwegianDay norwegian_tomorrow = now.IncrementNorwegianDaysCopy(1).AsNorwegianDay();

    //If we have not cached today, cache today immediately!!!
    if (most_recent_norwegian_today != norwegian_today)
    {
      if (::GetApp()->GetSpotprice()->CacheEurRates(norwegian_today) &&
          ::GetApp()->GetMQTT()->GotPrices(norwegian_today) &&
          ::GetApp()->GetSVG()->GenerateSVGs(norwegian_today))
      {
        most_recent_norwegian_today = norwegian_today;
      }
      else
      {
        Poco::Logger::get(Logger::DEFAULT).error("Caching spotprice for today failed");
        failed = true;
        continue;
      }
    }

    //According to Nordpool, final day-ahead bids must be submitted before 1200 CET, and "typically announced to the market at 12:42 CET or later". 1242 CET is 1142 UTC.
    //Try polling every 20 minute starting at 1200 UTC

    //Wait until at least 1200 UTC (unless caching of today failed. In that case, caching of today should be retried after a short pause)
    UTCTime poll_time;
    poll_time = poll_time.IncrementSecondsCopy(poll_time.GetNorwegianTimezoneOffset()); //Make sure we're at the correct norwegian day
    poll_time.SetTime(12, 0, 0);

    if (most_recent_norwegian_today==norwegian_today && most_recent_norwegian_tomorrow!=norwegian_tomorrow && now<poll_time)
    {
      std::unique_lock<std::mutex> lock(spotprice_mutex);
      if (spotprice_cv.wait_until(lock, token, std::chrono::system_clock::from_time_t(poll_time.AsUTCTimeT()), [token]{return token.stop_requested();}))
      {
        Poco::Logger::get(Logger::DEFAULT).warning("Stop requested");
        return;
      }
    }

    //After 1200 UTC, try to cache any not already cached prices up to 3 times an hour (at XX:00, XX:20 and XX:40)
    now = UTCTime();
    if (now >= poll_time)
    {
      if (most_recent_norwegian_today!=norwegian_today || most_recent_norwegian_tomorrow!=norwegian_tomorrow)
      {
        if (!first)
        {
          poll_time = UTCTime().IncrementSecondsCopy(20*60);
          poll_time.SetMinute(static_cast<uint8_t>((poll_time.GetMinute()/20)*20)); //Integer division to round down to 00|20|40
          poll_time.SetSecond(0);

          std::unique_lock<std::mutex> lock(spotprice_mutex);
          if (spotprice_cv.wait_until(lock, token, std::chrono::system_clock::from_time_t(poll_time.AsUTCTimeT()), [token]{return token.stop_requested();}))
          {
            Poco::Logger::get(Logger::DEFAULT).warning("Stop requested");
            return;
          }
        }
        
        //If, however unlikely, we have waited until midnight, restart loop iteration to reflect change of day
        if (UTCTime().AsNorwegianDay() != norwegian_today)
        {
          Poco::Logger::get(Logger::DEFAULT).information("SpotpriceCron passed midnight");
          continue;
        }
        
        first = false;
        
        //Are we STILL not having todays prices? Keep on trying..
        if (most_recent_norwegian_today != norwegian_today)
        {
          if (::GetApp()->GetSpotprice()->CacheEurRates(norwegian_today) &&
              ::GetApp()->GetMQTT()->GotPrices(norwegian_today) &&
              ::GetApp()->GetSVG()->GenerateSVGs(norwegian_today))
          {
            most_recent_norwegian_today = norwegian_today;
          }
          else
          {
            Poco::Logger::get(Logger::DEFAULT).error("Caching spotprice for today failed again");
            failed = true;
            continue;
          }
        }
        
        //And chech if Nordpool has published prices for tomorrow
        now = UTCTime();
        if (most_recent_norwegian_tomorrow != norwegian_tomorrow)
        {
          if (::GetApp()->GetSpotprice()->CacheEurRates(norwegian_tomorrow) &&
              ::GetApp()->GetMQTT()->GotPrices(norwegian_tomorrow) &&
              ::GetApp()->GetSVG()->GenerateSVGs(norwegian_tomorrow))
          {
            most_recent_norwegian_tomorrow = norwegian_tomorrow;
          }
          else
          {
            Poco::Logger::get(Logger::DEFAULT).error("Caching spotprice for tomorrow failed at " + now.AsNorwegianTime().ToString());
            failed = true;
            continue;
          }
        }
      }
      else //Eveything is done for today. Wait until midnight norwegian time (should even work for days with 23 or 25 hours)
      {
        now = UTCTime();
        NorwegianTime midnight_norwegiantime = now.AsNorwegianTime();
        UTCTime midnight_utc = now.IncrementHoursCopy(24 - midnight_norwegiantime.GetHour());
        midnight_utc.SetMinute(0);
        midnight_utc.SetSecond(0);

        std::unique_lock<std::mutex> lock(spotprice_mutex);
        if (spotprice_cv.wait_until(lock, token, std::chrono::system_clock::from_time_t(midnight_utc.AsUTCTimeT()), [token]{return token.stop_requested();}))
        {
          Poco::Logger::get(Logger::DEFAULT).warning("Stop requested");
          return;
        }
      }
    }
  }
}
