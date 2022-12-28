#include "application.h"

#include <thread>
#include <Poco/FileChannel.h>
#include <Poco/FormattingChannel.h>
#include <Poco/PatternFormatter.h>


Poco::AutoPtr<Elspot> g_application = nullptr;
void SetApp(const Poco::AutoPtr<Elspot> application) {g_application = application;}
Elspot* GetApp() {return g_application;}


Elspot::Elspot()
: Poco::Util::Application()
{
}

void Elspot::init(int /*argc*/, char* /*argv*/[])
{
  ::SetApp(this);
  m_config = new Poco::Util::PropertyFileConfiguration("elspot.properties");

  SetCurrency(std::make_shared<Currency>());
  SetMQTT(std::make_shared<MQTT>());
  SetSpotprice(std::make_shared<Spotprice>());
  SetSVG(std::make_shared<SVG>());
  SetNetworking(std::make_shared<Networking>());
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

  std::stop_source stop_source;
  std::jthread mqtt_cron_thread{mqtt_cron, stop_source.get_token()};
  std::jthread spotprice_cron_thread{spotprice_cron, stop_source.get_token()};
  return 0;
}

void Elspot::release()
{
}

std::string Elspot::GetConfig(const std::string& key) const
{
  return m_config->getString(key);
}
