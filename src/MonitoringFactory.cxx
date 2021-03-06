///
/// \file MonitoringFactory.cxx
/// \author Adam Wegrzynek <adam.wegrzynek@cern.ch>
///

#include "Monitoring/MonitoringFactory.h"
#include <functional>
#include <stdexcept>
#include <boost/algorithm/string.hpp>
#include "MonLogger.h"
#include "UriParser/UriParser.h"

#include "Backends/InfoLoggerBackend.h"
#include "Backends/StdOut.h"
#include "Backends/Flume.h"
#include "Backends/Noop.h"

#ifdef _WITH_APPMON
#include "Backends/ApMonBackend.h"
#endif

#include "Backends/InfluxDB.h"
#include "MonLogger.h"

namespace o2 
{
/// ALICE O2 Monitoring system
namespace monitoring 
{

std::unique_ptr<Backend> getInfoLogger(http::url uri) {
  if (uri.host == "") {
    return std::make_unique<backends::StdOut>();
  } else {
    return std::make_unique<backends::InfoLoggerBackend>(uri.host, uri.port);
  }
}

std::unique_ptr<Backend> getInfluxDb(http::url uri) {
  auto const position = uri.protocol.find_last_of('-');
  if (position != std::string::npos) {
    uri.protocol = uri.protocol.substr(position + 1);
  }
  if (uri.protocol == "udp") {
    return std::make_unique<backends::InfluxDB>(uri.host, uri.port);
  }
  if (uri.protocol == "http") {
    return std::make_unique<backends::InfluxDB>(uri.host, uri.port, uri.search);
  }
  throw std::runtime_error("InfluxDB transport protocol not supported");
}

#ifdef _WITH_APPMON
std::unique_ptr<Backend> getApMon(http::url uri) {
  return std::make_unique<backends::ApMonBackend>(uri.path);
}
#else
std::unique_ptr<Backend> getApMon(http::url /*uri*/) {
  throw std::runtime_error("ApMon backend is not enabled");
}
#endif

std::unique_ptr<Backend> getNoop(http::url /*uri*/) {
  return std::make_unique<backends::Noop>();
}

std::unique_ptr<Backend> getFlume(http::url uri) {
  return std::make_unique<backends::Flume>(uri.host, uri.port);
}

void MonitoringFactory::SetVerbosity(std::string selected, std::unique_ptr<Backend>& backend) {
  static const std::map<std::string, backend::Verbosity> verbosities = {
    {"/prod", backend::Verbosity::Prod},
    {"/debug", backend::Verbosity::Debug}
  };

  auto found = verbosities.find(selected);
  if (found != verbosities.end()) {
    backend->setVerbosisty(found->second);
    MonLogger::Get() << "...verbosity set to "
                     << static_cast<std::underlying_type<backend::Verbosity>::type>(found->second)
                     << MonLogger::End();
  }
}

std::unique_ptr<Backend> MonitoringFactory::GetBackend(std::string& url) {
  static const std::map<std::string, std::function<std::unique_ptr<Backend>(const http::url&)>> map = {
    {"infologger", getInfoLogger},
    {"influxdb-udp", getInfluxDb},
    {"influxdb-http", getInfluxDb},
    {"apmon", getApMon},
    {"flume", getFlume},
    {"no-op", getNoop}
  };

  http::url parsedUrl = http::ParseHttpUrl(url);
  if (parsedUrl.protocol.empty()) {
    throw std::runtime_error("Ill-formed URI");
  }   

  auto iterator = map.find(parsedUrl.protocol);
  if (iterator == map.end()) {
    throw std::runtime_error("Unrecognized backend " + parsedUrl.protocol);
  }

  auto backend = iterator->second(parsedUrl);
  if (!parsedUrl.path.empty()) {
    SetVerbosity(parsedUrl.path, backend);
  }
  return backend;
}

std::unique_ptr<Monitoring> MonitoringFactory::Get(std::string urlsString)
{
  auto monitoring = std::make_unique<Monitoring>();

  std::vector<std::string> urls;
  boost::split(urls, urlsString, boost::is_any_of(","));

  for (auto url : urls) {
    monitoring->addBackend(MonitoringFactory::GetBackend(url));
  }
  return monitoring;
}

} // namespace monitoring
} // namespace o2
