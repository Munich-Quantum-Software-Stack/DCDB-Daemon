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

* Build tools: CMake >= 3.19, C++17 compiler (g++ or clang++), libcurl (dev headers), libuv

Libraries — split into two groups:

**Managed in `extern/`** (cloned and built by the scripts described below):
  - [cpr](https://github.com/libcpr/cpr) v1.14.2 — C++ HTTP client
  - [influxdb-cxx](https://github.com/offa/influxdb-cxx) v0.8.1 — InfluxDB C++ client
  - [cpp-driver](https://github.com/datastax/cpp-driver) v2.17.1 — DataStax C/C++ Cassandra driver (`cassandra.h`); required by QDMI-Devices' DCDB backend (not available in Debian bookworm standard repos)
  - [QDMI-Devices](https://github.com/Munich-Quantum-Software-Stack/MQSS-QDMI-Devices-Suite) DCDB backend — provides `libdcdb_device.so`; DCDB itself is fetched and built automatically via cmake FetchContent when using `--with-dcdb`

**System prerequisites:**
  - `libuv1-dev`, `libssl-dev`, `libcurl` (dev headers) — available via `apt`

Runtime: a running DCDB deployment and an InfluxDB instance.

## Dependencies (`extern/`)

Two helper scripts in `extern/` automate cloning and building the managed dependencies.

**Step 0 — install system prerequisites (Debian/Ubuntu):**

```bash
sudo apt install -y libuv1-dev libssl-dev libcurl4-openssl-dev
```

**Step 1 — clone sources:**

```bash
# Core deps only (cpr + influxdb-cxx):
bash extern/clone.sh

# Also include QDMI-Devices and the Cassandra C++ driver:
bash extern/clone.sh --with-qdmi-devices
```

**Step 2 — build and install into `extern/installed/`:**

```bash
# Core deps only:
bash extern/build.sh

# With QDMI-Devices, letting cmake fetch and build DCDB automatically:
bash extern/build.sh --with-qdmi-devices --with-dcdb

# With QDMI-Devices, pointing to a pre-installed DCDB:
bash extern/build.sh --with-qdmi-devices --dcdb-root /path/to/DCDB
```

Use `bash extern/build.sh --help` for all options (e.g. `--jobs N`, `--force`).

After a successful build the layout is:

```
extern/
  installed/           ← cpr, influxdb-cxx, cassandra driver (cmake --install)
  QDMI-Devices/build/
    src/databases/dcdb/          ← libdcdb_device.so + headers (not installed)
    _deps/qdmi-src/include/      ← QDMI core headers (FetchContent)
    _deps/dcdb-src/lib/include/  ← DCDB headers (FetchContent)
    _deps/dcdb-build/lib/        ← libdcdb.so (FetchContent)
```

**Step 3 — export environment variables:**

All paths are computed automatically relative to the repo root:

```bash
source build/export_qdmi_dcdb_deps.sh
```

## Building

With the environment variables above set, configure and build:

```bash
cmake -S . -B build
cmake --build build -j
```

The daemon binary will appear at `build/mqss_dcdb_daemon`.

## Running

In the `build/` directory, there is a bash script showing a command example how to run the daemon. For instance,

```bash
./mqss_dcdb_daemon --config ../config.json --log-file daemon_log.txt --dcdb-url localhost:9042
```

where, `--config ../config.json` points to the config file containing a list of sensors, the database server, etc;
`--log-file daemon_log.txt` points to where we output the logs; `--dcdb-url localhost:9042` points to the server that we can collect/query sensor data.

## License

This project is released under the Apache License v2.0 with LLVM Exceptions. See [LICENSE](https://github.com/Munich-Quantum-Software-Stack/MQSS-CUDAQ-Adapter/blob/develop/LICENSE) for more information. Any contribution to the project is assumed to be under the same license.