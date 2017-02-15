///
/// \file MonInfoLogger.h
/// \author Barthelemy von Haller
/// \author Adam Wegrzynek <adam.wegrzynek@cern.ch>
///

#ifndef MONITORING_MONINFOLOGGER_H
#define MONITORING_MONINFOLOGGER_H

#include <iostream>
#include <InfoLogger/InfoLogger.hxx>

namespace AliceO2
{
/// ALICE O2 Monitoring system
namespace Monitoring
{

using namespace AliceO2::InfoLogger;

/// Singleton class that is used in all Monitoring classes
///
/// The aim of this class is to avoid every class in the package to define and configure its own instance of InfoLogger.
/// Independent InfoLogger instance can still be created when and if needed.
class MonInfoLogger : public AliceO2::InfoLogger::InfoLogger
{
  public:
    static MonInfoLogger &Get()
    {
      static MonInfoLogger infoLoggerInstance;
      return infoLoggerInstance;
    }

  private:
    MonInfoLogger()
    {
      *this << "InfoLogger instance for Monitoring initialized" << InfoLogger::endm;
    }

    /// Delete copy and move constructors
    MonInfoLogger &operator=(const MonInfoLogger &) = delete;
    MonInfoLogger(const MonInfoLogger &) = delete;
    MonInfoLogger(MonInfoLogger &&) = delete;
    MonInfoLogger &operator=(InfoLogger&&) = delete;

};

} // Monitoring
} // Alice02

#endif //MONITORING_MONINFOLOGGER_H
