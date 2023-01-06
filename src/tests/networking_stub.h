#ifndef _NETWORKING_STUB_H_
#define _NETWORKING_STUB_H_

#include <filesystem>

#include "../networking.h"


void fileContent(const std::filesystem::path& filepath, std::string& result);


class HTTPSClientSessionStub : public Poco::Net::HTTPSClientSession
{
public:
  HTTPSClientSessionStub();

public:
  std::istream& receiveResponse(Poco::Net::HTTPResponse& response) override;

  void setStubResponse(const std::string& response_text, const Poco::Net::HTTPResponse::HTTPStatus response_code);

private:
  std::string m_response_text;
  Poco::Net::HTTPResponse m_response;

  std::stringstream m_stream;
};


class NetworkingStub : public Networking
{
public:
  NetworkingStub(const std::string& spotprice_response_text, const Poco::Net::HTTPResponse::HTTPStatus spotprice_response_code,
                 const std::string& exchangerate_response_text, const Poco::Net::HTTPResponse::HTTPStatus exchangerate_response_code);

public:
  std::shared_ptr<Poco::Net::HTTPSClientSession> CreateSession(const Poco::URI& uri) const override;
  void CallGET(const std::shared_ptr<Poco::Net::HTTPSClientSession>& session, const Poco::URI& uri, const std::string& accept) const override;

private:
  const std::string m_spotprice_response_text;
  const Poco::Net::HTTPResponse::HTTPStatus m_spotprice_response_code;
  const std::string m_exchangerate_response_text;
  const Poco::Net::HTTPResponse::HTTPStatus m_exchangerate_response_code;
};

#endif // _NETWORKING_STUB_H_
