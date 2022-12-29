#include <gtest/gtest.h>

#include <chrono>

#include "networking_mock.h"
#include "spotprice_mock.h"

#include "../application.h"
#include "../spotprice_cron.h"


bool mockNetworkResponse(const std::string& mocked_response, const NorwegianDay& day, Spotprice::AreaRateType eur_rates)
{
  auto networking_mock = std::make_shared<testing::NiceMock<NetworkingMock>>();
  auto session_mock = std::make_unique<testing::NiceMock<HTTPSClientSessionMock>>();

  std::shared_ptr<Elspot> elspot = std::make_shared<Elspot>();
  elspot->init(0, nullptr);
  elspot->SetNetworking(networking_mock);

  Poco::Net::HTTPResponse dummy_httpresponse(Poco::Net::HTTPResponse::HTTP_OK);
  std::stringstream dummy_response(mocked_response);
  ON_CALL(*session_mock, receiveResponse(testing::_))
    .WillByDefault(testing::DoAll(
      testing::SetArgReferee<0>(dummy_httpresponse),
      testing::ReturnRef(dummy_response)));

  ON_CALL(*networking_mock, CreateSession(testing::_))
    .WillByDefault(testing::Return(testing::ByMove(std::move(session_mock))));

  ON_CALL(*networking_mock, CallGET(testing::_, testing::_, testing::_))
    .WillByDefault(testing::Return());

  std::stop_source stop_source;
  std::jthread spotprice_cron_thread{spotprice_cron, stop_source.get_token()};
  std::this_thread::sleep_for(std::chrono::seconds(1));
  stop_source.request_stop();

  elspot->SetNetworking(nullptr); //Decrease refCounter to make gMock do its magic

  return elspot->GetSpotprice()->GetEurRates(day, eur_rates);
}

TEST(SpotpriceCronTest, NotXMLResponseTest) {
  NorwegianDay dummy_day = UTCTime().AsNorwegianDay();
  Spotprice::AreaRateType eur_rates;
  EXPECT_FALSE(mockNetworkResponse("This is invalid XML", dummy_day, eur_rates));
}

TEST(SpotpriceCronTest, InvalidXMLResponseTest) {
  NorwegianDay dummy_day = UTCTime().AsNorwegianDay();
  Spotprice::AreaRateType eur_rates;
  EXPECT_FALSE(mockNetworkResponse("<xml>Invalid XML</xlm>", dummy_day, eur_rates));
}

TEST(SpotpriceCronTest, IncompleteXMLResponseTest) {
  NorwegianDay dummy_day = UTCTime().AsNorwegianDay();
  Spotprice::AreaRateType eur_rates;
  EXPECT_FALSE(mockNetworkResponse("<xml>Valid XML, with no rates</xml>", dummy_day, eur_rates));
}
