#ifndef _SPOTPRICE_MOCK_H_
#define _SPOTPRICE_MOCK_H_

#include <gmock/gmock.h>  // Brings in gMock.

#include "../spotprice.h"


class SpotpriceMock : public Spotprice
{
public:
  virtual ~SpotpriceMock() = default;

public:
  MOCK_METHOD(bool, HasEurRate,    (const NorwegianDay& norwegian_day), (const override));
  MOCK_METHOD(bool, CacheEurRates, (const NorwegianDay& norwegian_day), (override));
  MOCK_METHOD(bool, GetEurQuarterRates,   (const NorwegianDay& norwegian_day, AreaQuarterRateType& eur_quarter_rates), (override));
  MOCK_METHOD(bool, FetchEurQuarterRates, (const NorwegianDay& norwegian_day), (override));
  MOCK_METHOD(bool, RegisterFail,  (const NorwegianDay& norwegian_day), (override));
};

#endif // _SPOTPRICE_MOCK_H_
