#ifndef _MQTT_MOCK_H_
#define _MQTT_MOCK_H_

#include <gmock/gmock.h>  // Brings in gMock.

#include "../mqtt.h"


class MQTTMock : public MQTT
{
public:
  virtual ~MQTTMock() = default;

public:
  MOCK_METHOD(void, connection_lost, (const std::string& cause), (override));
  MOCK_METHOD(bool, GotPrices, (const NorwegianDay& norwegian_day));
  MOCK_METHOD(bool, PublishCurrentPrices, ());

};

#endif // _MQTT_MOCK_H_
