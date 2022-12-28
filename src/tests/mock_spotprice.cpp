#include <gtest/gtest.h>

#include <chrono>

#include "networking_mock.h"
#include "spotprice_mock.h"

#include "../application.h"
#include "../spotprice_cron.h"


TEST(SpotpriceCronTest, InvalidResponseTest) {
  auto networking_mock = std::make_shared<testing::NiceMock<NetworkingMock>>();
  auto session_mock = std::make_unique<testing::NiceMock<HTTPSClientSessionMock>>();

  Elspot* elspot = new Elspot();
  elspot->init(0, nullptr);
  elspot->SetNetworking(networking_mock);

  Poco::Net::HTTPResponse dummy_httpresponse(Poco::Net::HTTPResponse::HTTP_OK);
  std::stringstream dummy_response("asdf");
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

  NorwegianDay dummy_day = UTCTime().AsNorwegianDay();
  Spotprice::AreaRateType eur_rates;
  EXPECT_FALSE(elspot->GetSpotprice()->GetEurRates(dummy_day, eur_rates));
}
