#ifndef _APPLICATION_H_
#define _APPLICATION_H_

#include <Poco/Util/Application.h>
#include <Poco/Util/PropertyFileConfiguration.h>

#include "currency.h"
#include "logger.h"
#include "mqtt.h"
#include "mqtt_cron.h"
#include "networking.h"
#include "spotprice.h"
#include "spotprice_cron.h"
#include "svg.h"


class Elspot : public Poco::Util::Application
{
public:
  static constexpr const char* ENTSOE_TOKEN_PROPERTY = "entsoe";
  static constexpr const char* EXCHANGERATESAPI_TOKEN_PROPERTY = "exchangeratesapi";
  static constexpr const char* SVG_DIRECTORY_PROPERTY = "svg_dir";
  static constexpr const char* SVG_TEMPLATE_FILE = "svg_template_file";
  
public:
  Elspot();

  void init(int argc, char* argv[]);
  [[nodiscard]] Logger& logger();
  virtual int run();
  void release();
    
public:
  void SetCurrency(std::shared_ptr<Currency> currency) {m_currency = currency;}
  void SetMQTT(std::shared_ptr<MQTT> mqtt) {m_mqtt = mqtt;}
  void SetSpotprice(std::shared_ptr<Spotprice> spotprice) {m_spotprice = spotprice;}
  void SetSVG(std::shared_ptr<SVG> svg) {m_svg = svg;}
  void SetNetworking(std::shared_ptr<Networking> networking) {m_networking = networking;}

  [[nodiscard]] std::shared_ptr<Currency>   GetCurrency() const {return m_currency;}
  [[nodiscard]] std::shared_ptr<MQTT>       GetMQTT() const {return m_mqtt;}
  [[nodiscard]] std::shared_ptr<Spotprice>  GetSpotprice() const {return m_spotprice;}
  [[nodiscard]] std::shared_ptr<SVG>        GetSVG() const {return m_svg;}
  [[nodiscard]] std::shared_ptr<Networking> GetNetworking() const {return m_networking;}

  [[nodiscard]] std::string GetConfig(const std::string& key) const;

private:
  Logger m_logger;
  Poco::AutoPtr<Poco::Util::PropertyFileConfiguration> m_config;

  std::shared_ptr<Currency>   m_currency;
  std::shared_ptr<MQTT>       m_mqtt;
  std::shared_ptr<Spotprice>  m_spotprice;
  std::shared_ptr<SVG>        m_svg;
  std::shared_ptr<Networking> m_networking;
};

[[nodiscard]] Elspot* GetApp();

#endif // _APPLICATION_H_
