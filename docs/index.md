# MQSS QDMI-based DCDB Daemon {#mainpage}

<!-- IMPORTANT: Keep the label above — it sets this as the Doxygen main page. -->

\tableofcontents

## Overview

The **MQSS QDMI-based DCDB Daemon** is a lightweight C++ daemon that continuously queries
telemetry sensor data through the [QDMI-DCDB](https://github.com/Munich-Quantum-Software-Stack/MQSS-QDMI-Devices-Suite)
device library and writes the results into an [InfluxDB](https://www.influxdata.com/) time-series
database for long-term storage and visualization.

[DCDB](https://gitlab.lrz.de/dcdb/dcdb) (Data Center Data Base) is a modular, continuous, and
holistic monitoring framework targeted at HPC environments. The daemon acts as the bridge between
DCDB sensor nodes and downstream analytics tools such as Grafana.

### Key Features

- Continuous polling of DCDB telemetry sensors via the QDMI device interface.
- Configurable sensor list and query window via a JSON config file.
- Writes collected values to InfluxDB using the line protocol.
- Supports resume mode — picks up from the last queried timestamp across restarts.
- Flexible deployment as a standalone daemon or in a containerized environment.

## Architecture

```
+-------------------+       +--------------------------------+
| DCDB Sensor Nodes | <---> | QDMI-based DCDB Device Library |
+-------------------+       +--------------------------------+
                                           ^
                                           |
                                           v
 +-----------------+          +-----------------------------+
 |    InfluxDB     |  <--->   | MQSS QDMI-based DCDB Daemon |
 +-----------------+          +-----------------------------+
          ^
          |
          v
 Visualization / Analytics (Grafana, Chronograf, ...)
```

- **DCDB** exposes sensor data (power, temperature, environmental metrics, …).
- **QDMI-DCDB Device** is the adapter library that queries those sensors.
- **MQSS Daemon** polls the adapter and pushes results to InfluxDB.

## How to Use This Documentation

- [Getting Started](getting_started.md) — install prerequisites, build the dependencies, and compile the daemon.
- [Usage](usage.md) — command-line options, config file format, and runtime examples.
- [Reference](files.html) — full API reference generated from source comments.
