#ifndef _NETWORKING_MOCK_H_
#define _NETWORKING_MOCK_H_

#include <gmock/gmock.h>  // Brings in gMock.

#include "../networking.h"


class HTTPSClientSessionMock : public Poco::Net::HTTPSClientSession
{
public:
  virtual ~HTTPSClientSessionMock() = default;

public:
  MOCK_METHOD(std::istream&, receiveResponse, (Poco::Net::HTTPResponse& response), ());
};


class NetworkingMock : public Networking
{
public:
  virtual ~NetworkingMock() = default;

public:
  MOCK_METHOD(std::shared_ptr<Poco::Net::HTTPSClientSession>, CreateSession, (const Poco::URI& uri), (const));
  MOCK_METHOD(void,                                           CallGET,       (const std::shared_ptr<Poco::Net::HTTPSClientSession>& session,
                                                                              const Poco::URI& uri,
                                                                              const std::string& accept), (const));
};

#endif // _NETWORKING_MOCK_H_
