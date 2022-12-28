#include "networking.h"

#include <Poco/Net/HTTPRequest.h>


std::unique_ptr<Poco::Net::HTTPSClientSession> Networking::CreateSession(const Poco::URI& uri) const
{
  return std::make_unique<Poco::Net::HTTPSClientSession>(uri.getHost(), uri.getPort());
}

void Networking::CallGET(const std::unique_ptr<Poco::Net::HTTPSClientSession>& session, const Poco::URI& uri, const std::string& accept) const
{
  // prepare path
  std::string path(uri.getPathAndQuery());
  if (path.empty())
    path = "/";

  // send request
  Poco::Net::HTTPRequest req(Poco::Net::HTTPRequest::HTTP_GET, path, Poco::Net::HTTPMessage::HTTP_1_1);
  session->setKeepAliveTimeout(Poco::Timespan(30, 0));
  req.set("Accept", accept);
  session->sendRequest(req);
}
