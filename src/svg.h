#ifndef _SVG_H_
#define _SVG_H_

#include "day.h"
#include "spotprice.h"


class SVG {
public:
  [[nodiscard]] bool GenerateSVGs(const LocalDay& day) const;
private:
  [[nodiscard]] bool GenerateSVG(const std::string& svg_template, const LocalDay& day, const std::string& currency_name, const double& exchange_rate, const Spotprice::AreaRateType& area_rates, const std::array<Area,5>::size_type& area_index) const;
private:
  [[nodiscard]] double dceil(double v, int p) const;
  [[nodiscard]] double dfloor(double v, int p) const;
  [[nodiscard]] double rateToYPos(const double& rate, const double& min_rate, const double& max_rate) const;
};

#endif // _SVG_H_
