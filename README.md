## MQSS QDMI-based DCDB Daemon

A lightweight C++ daemon that continuously queries telemetry sensor data through the QDMI‑DCDB device library, in which [DCDB](https://gitlab.lrz.de/dcdb/dcdb) refers to Data Center Data Base, a modular, continuous, and holistic monitoring framework targeted at HPC environments. The daemon writes relevant sensor data into an InfluxDB time-series database and exposes data for use in dashboards or other downstream analytics.

The daemon hase following features:
* Continuous querying data of DCDB telemetry sensors.
* Connection management using the QDMI‑DCDB library.
* Writes queried values into InfluxDB for long‑term storage and visualization (Grafana, Chronograf, etc.).
* Flexible deployment as standalone daemon.

Architecture overview
```Bash
+-------------------+       +--------------------------------+
| DCDB Sensor Nodes | <---> | QDMI‑based DCDB Device Library |
+-------------------+       +--------------------------------+
                                           ^
                                           |
                                           v
 +-----------------+          +-----------------------------+
 |    InfluxDB     |  <--->   | MQSS QDMI-based DCDC Daemon |
 +-----------------+          +-----------------------------+
          ^
          |
          v
 Visualization / Analytics (Grafana, Custom Tools, ...)
```

* DCDB provides sensor data (power, temperature, environment metrics, ...)
* QDMI‑DCDB Device as a device adapter to query those metrics.
* MQSS Daemon polls the metrics and pushes them to InfluxDB.

## Requirements

* Build tools: CMake >= 3.19, C++17 compiler (g++ or clang++)

* Libraries (installed or built from source):
  - DCDB core libraries (`libdcdb.so` or `libdcdbclient.so`, headers).
  - QDMI‑Devices DCDB Device Lib (`libdcdb_device.so`).

* Runtime: access to a DCDB deployment and an InfluxDB instance might depend on the HPC-center-specific configuration.

```Bash
export DCDB_LIB_DIR=/path/to/DCDB/install/lib
export DCDB_INC_DIR=/path/to/DCDB/install/include
export DCDB_COMMON_DIR=/path/to/DCDB/dcdb/common
export QDMI_DCDB_DEV_LIB_DIR=/path/to/QDMI-Devices/build/src/databases/dcdb
export QDMI_DCDB_DEV_INC_DIR=/path/to/QDMI-Devices/build/src/databases/dcdb/include
export QDMI_INC_DIR=/path/to/QDMI-Devices/build/_deps/qdmi-src/include
```

## Building

Basic build can be: `cmake -S . -B build`, then `cmake --build build -j`. The daemon binary will appear at `build/mqss_dcdb_daemon`.

## License

This project is released under the Apache License v2.0 with LLVM Exceptions. See [LICENSE](https://github.com/Munich-Quantum-Software-Stack/MQSS-CUDAQ-Adapter/blob/develop/LICENSE) for more information. Any contribution to the project is assumed to be under the same license.