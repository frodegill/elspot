#ifndef _LOGGER_H_
#define _LOGGER_H_

#include <Poco/Exception.h>


class Logger
{
public:
  static constexpr const char* DEFAULT = "default";

public:
    void log(Poco::Exception& ex);
};

#endif // _LOGGER_H_
