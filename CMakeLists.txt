####################################
# General project definition
####################################

CMAKE_MINIMUM_REQUIRED(VERSION 3.5.2 FATAL_ERROR)

# Set cmake policy by version: https://cmake.org/cmake/help/latest/manual/cmake-policies.7.html
if(${CMAKE_VERSION} VERSION_LESS 3.12)
    cmake_policy(VERSION ${CMAKE_MAJOR_VERSION}.${CMAKE_MINOR_VERSION})
else()
    cmake_policy(VERSION 3.12)
endif()

# Define project
project(Monitoring
  VERSION 1.9.2
  DESCRIPTION "O2 Monitoring library"
  LANGUAGES CXX
)

# Documentation dir
add_subdirectory(doc)

# Add compiler flags for warnings and debug symbols
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -pedantic -Wextra")

# Set fPIC for all targets
set(CMAKE_POSITION_INDEPENDENT_CODE ON)

# Set the default build type to "RelWithDebInfo"
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE "RelWithDebInfo"
    CACHE
    STRING "Choose the type of build, options are: Debug Release RelWithDebInfo MinSizeRel Coverage."
    FORCE
  )
endif()

# Add coverage flags to Debug build
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g -O0 --coverage")
endif()

####################################
# Dependencies
####################################

list(APPEND CMAKE_MODULE_PATH "${CMAKE_CURRENT_LIST_DIR}/cmake")

find_package(Boost REQUIRED COMPONENTS unit_test_framework program_options system filesystem)
find_package(Git QUIET)
find_package(ApMon MODULE)
find_package(CURL REQUIRED MODULE)


####################################
# Library
####################################

set(LIBRARY_OUTPUT_PATH "${CMAKE_BINARY_DIR}/lib")
set(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIR}/bin")
set(INCLUDE_OUTPUT_DIRECTORY "${PROJECT_BINARY_DIR}/include")

set(SRCS
  src/Monitoring.cxx
  src/Metric.cxx
  src/Backends/InfoLoggerBackend.cxx
  src/Backends/InfluxDB.cxx
  src/Backends/Flume.cxx
  src/Backends/StdOut.cxx
  src/DerivedMetrics.cxx
  src/ProcessMonitor.cxx
  src/ProcessDetails.cxx
  src/MonitoringFactory.cxx
  src/Transports/UDP.cxx
  src/Transports/TCP.cxx
  src/Transports/HTTP.cxx
  src/Exceptions/MonitoringException.cxx
)

# Backends
message(STATUS "Backends")
message(STATUS "  Compiling InfoLoggerBackend backend")
message(STATUS "  Compiling Flume UDP/JSON backend")
message(STATUS "  Compiling InfluxDB HTTP and UDP backend")

# Create library
add_library(Monitoring SHARED ${SRCS} $<$<BOOL:${ApMon_FOUND}>:src/Backends/ApMonBackend.cxx>)
target_include_directories(Monitoring
  PUBLIC 
    $<INSTALL_INTERFACE:include>    
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

# Link targets
target_link_libraries(Monitoring
  PUBLIC
    Boost::boost
  PRIVATE
    Boost::system CURL::CURL
)

# Handle ApMon optional dependency
if(ApMon_FOUND)
  message(STATUS "  Compiling ApMon backend")
endif()

# Detect operating system
if (UNIX AND NOT APPLE)
  message(STATUS "Detected Linux: Process monitor enabled")
  set(LINUX true)
endif()

if (APPLE)
  message(STATUS "Detected macOS: Process monitor disabled")
endif()

# Handle custom compile definitions
target_compile_definitions(Monitoring
  PRIVATE
    $<$<BOOL:${APPLE}>:_OS_MAC>
    $<$<BOOL:${LINUX}>:_OS_LINUX>
    $<$<BOOL:${ApMon_FOUND}>:_WITH_APPMON>
  )

# Use C++14
target_compile_features(Monitoring PUBLIC cxx_std_14)

# Handle optional ApMon dependency
if(ApMon_FOUND)
  target_link_libraries(Monitoring
    PRIVATE
      ApMon::ApMon
  )
endif()


####################################
# Executables
####################################

set(EXAMPLES
  examples/1-Basic.cxx
  examples/2-TaggedMetrics.cxx
  examples/3-UserDefinedTimestamp
  examples/4-RateDerivedMetric.cxx
  examples/5-Benchmark.cxx
  examples/6-Increment.cxx
  examples/7-Latency.cxx
  examples/8-Multiple.cxx
  examples/9-Timer.cxx
  examples/10-Buffering.cxx
  examples/11-AutoUpdate.cxx
)

foreach (example ${EXAMPLES})
  get_filename_component(example_name ${example} NAME)
  string(REGEX REPLACE ".cxx" "" example_name ${example_name})
  add_executable(${example_name} ${example})
  target_link_libraries(${example_name}
    PRIVATE
      Monitoring Boost::program_options
  )
endforeach()


####################################
# Tests
####################################

enable_testing()

set(TEST_SRCS
  test/testMonitoring.cxx
  test/testMonitoringFactory.cxx
  test/testDerived.cxx
  test/testFlume.cxx
  test/testMetric.cxx
  test/testProcessDetails.cxx
  test/testProcessMonitor.cxx
  test/testInfluxDb.cxx
  test/testNoop.cxx
)

if(ApMon_FOUND)
  list(APPEND TEST_SRCS test/testApMon.cxx)
  configure_file(test/ApMon.conf ${CMAKE_CURRENT_BINARY_DIR}/ApMon.conf COPYONLY)
endif()

foreach (test ${TEST_SRCS})
  get_filename_component(test_name ${test} NAME)
  string(REGEX REPLACE ".cxx" "" test_name ${test_name})

  add_executable(${test_name} ${test})
  target_link_libraries(${test_name}
    PRIVATE 
      Monitoring Boost::unit_test_framework Boost::filesystem
  )
  add_test(NAME ${test_name} COMMAND ${test_name})
  set_tests_properties(${test_name} PROPERTIES TIMEOUT 60)
endforeach()


####################################
# Install
####################################

include(GNUInstallDirs)

# Build targets with install rpath on Mac to dramatically speed up installation
# https://gitlab.kitware.com/cmake/community/wikis/doc/cmake/RPATH-handling
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
list(FIND CMAKE_PLATFORM_IMPLICIT_LINK_DIRECTORIES "${CMAKE_INSTALL_PREFIX}/lib" isSystemDir)
if(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
  if("${isSystemDir}" STREQUAL "-1")
    set(CMAKE_INSTALL_RPATH "@loader_path/../lib")
  endif()
  set(CMAKE_BUILD_WITH_INSTALL_RPATH TRUE)
endif()
unset(isSystemDir)

# Install library
install(TARGETS Monitoring
  EXPORT MonitoringTargets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
)

# Create version file
include(CMakePackageConfigHelpers)
write_basic_package_version_file("${CMAKE_CURRENT_BINARY_DIR}/cmake/MonitoringConfigVersion.cmake"
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY AnyNewerVersion
)

# Install headers
install(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/include/Monitoring DESTINATION "${CMAKE_INSTALL_INCLUDEDIR}")

# Export targets
install(EXPORT MonitoringTargets
  FILE
    MonitoringTargets.cmake
  NAMESPACE
    AliceO2::
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/Monitoring
)

# Configure and install Config files
configure_package_config_file(
  cmake/MonitoringConfig.cmake.in cmake/MonitoringConfig.cmake
  INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/Monitoring"
  PATH_VARS CMAKE_INSTALL_PREFIX
)

install(
  FILES
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/MonitoringConfig.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/cmake/MonitoringConfigVersion.cmake"
  DESTINATION
    ${CMAKE_INSTALL_LIBDIR}/cmake/Monitoring
)
