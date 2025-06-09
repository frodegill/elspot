#include "mqtt_cron.h"

#include "application.h"
#include "day.h"


void mqtt_cron(std::stop_token token)
{
  while(!token.stop_requested())
  {
    //Wait until next quarter
    UTCTime this_quarter;
    this_quarter.SetMinute((this_quarter.GetMinute()/15)*15); //Round down to current quarter
    this_quarter.SetSecond(0);
    UTCTime next_quarter = this_quarter.IncrementMinutesCopy(15);
    std::this_thread::sleep_until(std::chrono::system_clock::from_time_t(next_quarter.AsUTCTimeT()));
    
    //Publish this quarter spotprices (and retry after 1 minute if it fails. (Give up after 10 minutes of retrying..)
    for (unsigned int retry_count=0; retry_count<10; retry_count++)
    {
      if (::GetApp()->GetMQTT()->PublishCurrentPrices())
      {
        break; //Success! Bail out
      }
      else {
        Poco::Logger::get(Logger::DEFAULT).information("MQTT publish failed. Wait 1 minute, retry");
        std::this_thread::sleep_for(std::chrono::minutes(1)); //Wait 1 minute, retry
      }
    }
  }
}
