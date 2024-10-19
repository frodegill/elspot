#include <gtest/gtest.h>

#include <chrono>
#include <filesystem>
#include <fstream>

#include "mqtt_mock.h"
#include "networking_stub.h"
#include "spotprice_mock.h"

#include "../application.h"
#include "../spotprice_cron.h"


bool stubNetworkResponse(const std::shared_ptr<NetworkingStub>& networking_stub, const NorwegianDay& day, Spotprice::AreaRateType& eur_rates)
{
  auto elspot = std::make_shared<Elspot>();
  elspot->init(0, nullptr);
  elspot->SetNetworking(networking_stub);
  auto mqtt_mock = std::make_shared<testing::NiceMock<MQTTMock>>();
  elspot->SetMQTT(mqtt_mock);

  EXPECT_CALL(*mqtt_mock, GotPrices(testing::_)).WillRepeatedly(testing::Return(true));

  std::stop_source stop_source;
  std::jthread spotprice_cron_thread{spotprice_cron, stop_source.get_token()};
  std::this_thread::sleep_for(std::chrono::seconds(1));
  stop_source.request_stop();

  return elspot->GetSpotprice()->GetEurRates(day, eur_rates);
}

TEST(SpotpriceCronTest, NotXMLResponseTest) {
  NorwegianDay dummy_day = UTCTime().AsNorwegianDay();
  Spotprice::AreaRateType eur_rates;
  auto networking_stub = std::make_shared<NetworkingStub>(std::string("This is invalid XML"), Poco::Net::HTTPResponse::HTTP_OK,
                                                          std::string("This is invalid XML"), Poco::Net::HTTPResponse::HTTP_OK);
  EXPECT_FALSE(stubNetworkResponse(networking_stub, dummy_day, eur_rates));
}

TEST(SpotpriceCronTest, InvalidXMLResponseTest) {
  NorwegianDay dummy_day = UTCTime().AsNorwegianDay();
  Spotprice::AreaRateType eur_rates;
  auto networking_stub = std::make_shared<NetworkingStub>(std::string("<xml>Invalid XML</xlm>"), Poco::Net::HTTPResponse::HTTP_OK,
                                                          std::string("<xml>Invalid XML</xlm>"), Poco::Net::HTTPResponse::HTTP_OK);
  EXPECT_FALSE(stubNetworkResponse(networking_stub, dummy_day, eur_rates));
}

TEST(SpotpriceCronTest, IncompleteXMLResponseTest) {
  NorwegianDay dummy_day = UTCTime().AsNorwegianDay();
  Spotprice::AreaRateType eur_rates;
  auto networking_stub = std::make_shared<NetworkingStub>(std::string("<xml>Valid XML, with no rates</xml>"), Poco::Net::HTTPResponse::HTTP_OK,
                                                          std::string("<xml>Valid XML, with no rates</xml>"), Poco::Net::HTTPResponse::HTTP_OK);
  EXPECT_FALSE(stubNetworkResponse(networking_stub, dummy_day, eur_rates));
}

TEST(SpotpriceCronTest, ToSummertimeXMLResponseTest) {
  NorwegianDay dummy_day = UTCTime(1648375200).AsNorwegianDay();
  Spotprice::AreaRateType eur_rates;
  std::string spotprice_response_text, exchangerate_response_text;
  fileContent(std::filesystem::path("src/tests/to_summertime.xml"), spotprice_response_text);
  fileContent(std::filesystem::path("src/tests/exchangerates.json"), exchangerate_response_text);
  auto networking_stub = std::make_shared<NetworkingStub>(spotprice_response_text, Poco::Net::HTTPResponse::HTTP_OK,
                                                          exchangerate_response_text, Poco::Net::HTTPResponse::HTTP_OK);
  EXPECT_TRUE(stubNetworkResponse(networking_stub, dummy_day, eur_rates));
  EXPECT_EQ(eur_rates[0][0], 194.84);
}

TEST(SpotpriceCronTest, ToWintertimeXMLResponseTest) {
  NorwegianDay dummy_day = UTCTime(1667127600).AsNorwegianDay();
  Spotprice::AreaRateType eur_rates;
  std::string spotprice_response_text, exchangerate_response_text;
  fileContent(std::filesystem::path("src/tests/to_wintertime.xml"), spotprice_response_text);
  fileContent(std::filesystem::path("src/tests/exchangerates.json"), exchangerate_response_text);
  auto networking_stub = std::make_shared<NetworkingStub>(spotprice_response_text, Poco::Net::HTTPResponse::HTTP_OK,
                                                          exchangerate_response_text, Poco::Net::HTTPResponse::HTTP_OK);
  EXPECT_TRUE(stubNetworkResponse(networking_stub, dummy_day, eur_rates));
  EXPECT_EQ(eur_rates[0][7], 106.63);
}

TEST(SpotpriceCronTest, CurveTypeA03XMLResponseTest) {
  NorwegianDay dummy_day = UTCTime(1729287162).AsNorwegianDay();
  Spotprice::AreaRateType eur_rates;
  std::string spotprice_response_text, exchangerate_response_text;
  fileContent(std::filesystem::path("src/tests/curvetype_a03.xml"), spotprice_response_text);
  fileContent(std::filesystem::path("src/tests/exchangerates.json"), exchangerate_response_text);
  auto networking_stub = std::make_shared<NetworkingStub>(spotprice_response_text, Poco::Net::HTTPResponse::HTTP_OK,
                                                          exchangerate_response_text, Poco::Net::HTTPResponse::HTTP_OK);
  EXPECT_TRUE(stubNetworkResponse(networking_stub, dummy_day, eur_rates));
  EXPECT_EQ(eur_rates[0][13], 13.17);
  EXPECT_EQ(eur_rates[0][14], 13.17);
  EXPECT_EQ(eur_rates[0][15], 13.42);
}
