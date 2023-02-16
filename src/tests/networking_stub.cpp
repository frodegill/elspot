#include "networking_stub.h"

#include <fstream>

#include <Poco/Logger.h>
#include "../logger.h"


void fileContent(const std::filesystem::path& filepath, std::string& result)
{
  result = "";
  std::ifstream file(filepath, std::ios::binary);
  if (file.is_open())
  {
    std::stringstream filestream;
    filestream << file.rdbuf();
    file.close();

    result = filestream.str();
  }
}

HTTPSClientSessionStub::HTTPSClientSessionStub()
: Poco::Net::HTTPSClientSession()
{
}

std::istream& HTTPSClientSessionStub::receiveResponse(Poco::Net::HTTPResponse& response)
{
  m_stream = std::stringstream(m_response_text);
  response = m_response;
  return m_stream;
}

void HTTPSClientSessionStub::setStubResponse(const std::string& response_text, const Poco::Net::HTTPResponse::HTTPStatus response_code)
{
  m_response_text = response_text;
  m_response = Poco::Net::HTTPResponse(response_code);
}


NetworkingStub::NetworkingStub(const std::string& spotprice_response_text, const Poco::Net::HTTPResponse::HTTPStatus spotprice_response_code,
                               const std::string& exchangerate_response_text, const Poco::Net::HTTPResponse::HTTPStatus exchangerate_response_code)
: m_spotprice_response_text(spotprice_response_text),
  m_spotprice_response_code(spotprice_response_code),
  m_exchangerate_response_text(exchangerate_response_text),
  m_exchangerate_response_code(exchangerate_response_code)
{
}

std::shared_ptr<Poco::Net::HTTPSClientSession> NetworkingStub::CreateSession(const Poco::URI&) const
{
  return std::make_shared<HTTPSClientSessionStub>();
}

void NetworkingStub::CallGET(const std::shared_ptr<Poco::Net::HTTPSClientSession>& session, const Poco::URI& uri, const std::string&) const
{
  Poco::Logger::get(Logger::DEFAULT).error(std::string("Stubbing GET for ") + uri.toString());
  if (uri.toString().starts_with("https://web-api.tp.entsoe.eu/"))
  {
    static_cast<HTTPSClientSessionStub*>(session.get())->setStubResponse(m_spotprice_response_text, m_spotprice_response_code);
  }
  else if (uri.toString().starts_with("http://api.exchangeratesapi.io/"))
  {
    static_cast<HTTPSClientSessionStub*>(session.get())->setStubResponse(m_exchangerate_response_text, m_exchangerate_response_code);
  }
}
