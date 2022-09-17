#include "application.h"

#include <thread>
#include <Poco/FileChannel.h>
#include <Poco/FormattingChannel.h>
#include <Poco/PatternFormatter.h>


Poco::AutoPtr<Elspot> g_application;
Elspot* GetApp() {return g_application;}


Elspot::Elspot()
: Poco::Util::Application()
{
}

void Elspot::init(int /*argc*/, char* /*argv*/[])
{
  g_application = this;
  m_config = new Poco::Util::PropertyFileConfiguration("elspot.properties");

  m_currency = std::make_shared<Currency>();
  m_mqtt = std::make_shared<MQTT>();
  m_spotprice = std::make_shared<Spotprice>();
  m_svg = std::make_shared<SVG>();
}

Logger& Elspot::logger()
{
  return m_logger;
}

int Elspot::run()
{
  Poco::AutoPtr<Poco::FileChannel> log_file(new Poco::FileChannel("/tmp/elspot.log"));
  log_file->setProperty("rotateOnOpen","false");
  Poco::AutoPtr<Poco::Formatter> log_formatter(new Poco::PatternFormatter("%d-%m-%Y %H:%M:%S %s: %t"));
  log_formatter->setProperty("times","local");
  Poco::AutoPtr<Poco::Channel> log_formattingchannel(new Poco::FormattingChannel(log_formatter,log_file));
  Poco::Logger::get(Logger::DEFAULT).setChannel(log_formattingchannel);
  Poco::Logger::get(Logger::DEFAULT).setLevel(Poco::Message::PRIO_INFORMATION);

  std::thread mqtt_cron(&MQTTCron::main, m_mqtt_cron);
  std::thread spotprice_cron(&SpotpriceCron::main, m_spotprice_cron);

  mqtt_cron.join(); //join will never return
  spotprice_cron.join();
  return 0;
}

void Elspot::release()
{
}

std::string Elspot::GetConfig(const std::string& key) const
{
  return m_config->getString(key);
}
