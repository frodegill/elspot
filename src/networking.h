#ifndef _NETWORKING_H_
#define _NETWORKING_H_

#include <memory>

#include <Poco/URI.h>
#include <Poco/Net/HTTPResponse.h>
#include <Poco/Net/HTTPSClientSession.h>


class Networking
{
public:
  virtual ~Networking() = default;

public:
  [[nodiscard]] virtual std::shared_ptr<Poco::Net::HTTPSClientSession> CreateSession(const Poco::URI& uri) const;
  virtual void CallGET(const std::shared_ptr<Poco::Net::HTTPSClientSession>& session, const Poco::URI& uri, const std::string& accept) const;

};

#endif // _NETWORKING_H_
