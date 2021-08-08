#include "mqtt_cron.h"

#include "application.h"
#include "day.h"


void MQTTCron::main()
{
  while(true)
  {
    //Wait until next whole hour
    UTCTime this_hour;
    this_hour.SetMinute(0);
    this_hour.SetSecond(0);
    UTCTime next_hour = this_hour.Increment(60*60);
    std::this_thread::sleep_until(std::chrono::system_clock::from_time_t(next_hour.AsUTCTimeT()));
    
    //Publish this hour spotprices (and retry after 5 minutes if it fails. (Give up after 50 minutes of retrying..)
    for (int retry_count=0; retry_count<10; retry_count++)
    {
      if (::GetApp()->getMQTT()->PublishCurrentPrices())
      {
        break; //Success! Bail out
      }
      else {
        Poco::Logger::get(Logger::DEFAULT).information("MQTT publish failed. Wait 5 minutes, retry");
        std::this_thread::sleep_for(std::chrono::minutes(5)); //Wait 5 minutes, retry
      }
    }
  }
}
