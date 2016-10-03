# Monitoring module

Monitoring module allows to inject user specific metric and monitor running processes running.

## Metrics
Metric consists of 4 parameters: name, value, entity and timestamp.

| Parameter name | Type                                          | Required | Default          |
| -------------- |:---------------------------------------------:|:--------:| ----------------:|
| name           | string                                        | yes      | -                |
| value          | int/double/uint32t/string                     | yes      | -                |
| entity         | string                                        | no       | <hostname>.<PID> |
| timestamp      | chrono::time_point&lt;std::chrono::system_clock&gt; | no       | current time     |

Not required parameters can be omitted as they are filled by default value.


## Processes monitoring
The running process is monitored by default. In addition any other processes running at the same machine can be monitored.
The following metrics are generated for each process:
1. cputime - cumulative CPU time, "[DD-]HH:MM:SS" format.
2. etime - elapsed time since the process was started, in the form [[DD-]hh:]mm:ss.
3. pcpu - cpu utilization of the process in "##.#" format. Currently, it is the CPU time used divided by the time the process has been running (cputime/realtime ratio), expressed as a percentage.  It will not add up to 100% unless you are lucky.
4. pmem - ratio of the process's resident set size  to the physical memory on the machine, expressed as a percentage.
5. rsz - resident set size, the non-swapped physical memory that a task has used (in kiloBytes).
6. vsz - virtual memory size of the process in KiB (1024-byte units).  Device mappings are currently excluded; this is subject to change.


## Monitoring backends
Metrics are pushed to one or multiple backends. The library currently supports three  backends.

| Backend name     | Description                                                 | Client-side requirements
| ---------------- |:-----------------------------------------------------------:| ----------------------------:|
| InfluxDB         | pushes metrics using cURL to InfluxDB time series database  | cURL: yum install libcurl    |
| ApMon (MonALISA) | pushes metrics via UDP to MonALISA Serivce                  | ApMon: yum install ApMon_cpp |
| InfoLogger       | O2 Logging module                                           | none                         |

Instruction how to install and configure server-sides backends are available in "Server-side backend installation and configuration" chapter.

## Configuration file
+ AppMon
  + enable - enable AppMon (MonALISA) backend
  + pathToConfig - path to AppMon configuration file
+ InfluxDB
  + enable - enable InfluxDB backend
  + hostname - server hostname
  + port - server port
  + db - name of database
+ InfoLoggerBackend
  + enable - enable InfoLogger backend
+ ProcessMonitor
  + enable - enable process monitor
  + interval - updates interval 
+ DerivedMetrics
  + maxCacheSize - maximum size of vector

See sample in examples/SampleConfig.ini

## Getting started
Examples are available in *example* directory.

### Injecting user specific metric
Simple example that allows to send a metric.
```cpp
// create monitoring object, pass confuguration path as parameter
  std::unique_ptr<Monitoring::Core::Collector> collector(
    new Monitoring::Core::Collector("file:///home/hackathon/Monitoring/SampleConfig.ini")
  );  

  // now send an application specific metric
  // 10 is the value
  // myCrazyMetric is the name of the metric
  collector->send(10, "myCrazyMetric");
```

### Non-default entity value
Default entity value is set to &lt;hostname&gt;.&lt;PID&gt; This value can be changed by calling setEntity method.
```cpp
  // create monitoring object, pass configuration path as parameter
  std::unique_ptr<Monitoring::Core::Collector> collector(
    new Monitoring::Core::Collector("file:///home/hackathon/Monitoring/SampleConfig.ini")
  );  

  // set user specific entity value, eg.:
  collector->setEntity("testEntity");

  // now send an application specific metric
  // 10 is the value
  // myCrazyMetric is the name of the metric
  collector->send(10, "myCrazyMetric");
```

### User-generated timestamp
By default timestamp is set by the library, but user can overwrite it manually.
```cpp
  // create monitoring object, pass configuration path as parameter
  std::unique_ptr<Monitoring::Core::Collector> collector(
    new Monitoring::Core::Collector("file:///home/hackathon/Monitoring/SampleConfig.ini")
  );  

  // generate current timestamp
  std::chrono::time_point<std::chrono::system_clock> timestamp = std::chrono::system_clock::now();
     
  // now send an application specific metric
  // 10 is the value 
  // myCrazyMetric is the name of the metric
  collector->send(10, "myCrazyMetric", timestamp);
  
  // sleep 1 second, and send different metric with the same timestamp
  std::this_thread::sleep_for(std::chrono::seconds(1));
  collector->send(40, "myCrazyMetric", timestamp);
}
```

###  Derived metrics
Library can calculate derived metrics: average value and rate.
```cpp
// create monitoring object, pass configuration path as parameter
  std::unique_ptr<Monitoring::Core::Collector> collector(
    new Monitoring::Core::Collector("file:///home/hackathon/Monitoring/SampleConfig.ini")
  );  

  // derived metric :  rate
  collector->addDerivedMetric("myCrazyMetric1", Monitoring::Core::DerivedMetricMode::RATE);

  // now send at least two metrics to see the result
  collector->send(10, "myCrazyMetric1");
  collector->send(20, "myCrazyMetric1");
  collector->send(30, "myCrazyMetric1");
  collector->send(50, "myCrazyMetric1");
```
### Process monitoring
Library monitors current process by default and allows to monitor other processes running at the same machine.
The updates are performed automatically at regular intervals of time (see Configuration file) but can also be executed manually. 

```cpp
  // create monitoring object, pass configuration path as parameter
  std::unique_ptr<Monitoring::Core::Collector> collector(
    new Monitoring::Core::Collector("file:///home/hackathon/Monitoring/SampleConfig.ini")
  );  

  // add additional PID to be monitored
  collector->addMonitoredPid(1);
     
  for (;;) {
    collector->send(10, "mainThreadMetric");
    // additional manual update
    collector->sendProcessMonitorValues();
    std::this_thread::sleep_for(std::chrono::seconds(1));
  }
```

## Server-side backend installation and configuration

### MonALISA
##### MonALISA Service
To install and configure the MonALISA service (1 central server):
+ yum install monalisa-service
+ copy config/ml_env to /opt/monalisa-service/Service/CMD/
+ copy config/ml.properties to /opt/monalisa-service/Service/myFarm/
+ copy config/myFarm.conf to /opt/monalisa-service/Service/myFarm/
+ add following line to iptables and restart it: -A INPUT -p udp -m state --state NEW -m udp --dport 8884 -m comment --comment "MonALISA UDP packets" -j ACCEPT
+ /sbin/service MLD start

##### MonALISA Client
To install the MonALISA Client run:
+ yum install monalisa-client

### InfluxDB
Instructions are avaliable here: https://docs.influxdata.com/influxdb/v1.0/introduction/installation/

### InfoLogger
There is no installation/configuration procedure needed. InfoLogger backend is available all the time (if not disabled in configuration file).
