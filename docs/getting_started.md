# Getting Started {#getting_started}

<!-- This file is a static page included in docs/CMakeLists.txt. -->

\tableofcontents

## Requirements {#requirements}

**Build tools:**
- CMake ≥ 3.19
- C++17 compiler (`g++` or `clang++`)

**System libraries (Debian/Ubuntu):**

```bash
sudo apt install -y libuv1-dev libssl-dev libcurl4-openssl-dev
```

**Runtime:**
- A running [DCDB](https://gitlab.lrz.de/dcdb/dcdb) deployment (Cassandra backend, default port 9042).
- A running [InfluxDB](https://www.influxdata.com/) instance reachable from the daemon host.

## Dependencies (`extern/`) {#dependencies}

Managed dependencies are cloned and built by two helper scripts in `extern/`.
They are installed into `extern/installed/` and never need to be installed system-wide.

| Library | Version | Purpose |
|---|---|---|
| [cpr](https://github.com/libcpr/cpr) | 1.14.2 | C++ HTTP client |
| [influxdb-cxx](https://github.com/offa/influxdb-cxx) | 0.8.1 | InfluxDB C++ client |
| [cpp-driver](https://github.com/datastax/cpp-driver) | 2.17.1 | Cassandra C/C++ driver (required by QDMI-Devices) |
| [QDMI-Devices](https://github.com/Munich-Quantum-Software-Stack/MQSS-QDMI-Devices-Suite) | — | DCDB backend (`libdcdb_device.so`) |

### Step 1 — Clone sources {#clone}

```bash
# Core deps only (cpr + influxdb-cxx):
bash extern/clone.sh

# Also include QDMI-Devices and the Cassandra driver:
bash extern/clone.sh --with-qdmi-devices
```

### Step 2 — Build and install {#build-deps}

```bash
# Core deps only:
bash extern/build.sh

# With QDMI-Devices — let CMake fetch and build DCDB automatically:
bash extern/build.sh --with-qdmi-devices --with-dcdb

# With QDMI-Devices — point to a pre-installed DCDB:
bash extern/build.sh --with-qdmi-devices --dcdb-root /path/to/DCDB
```

Use `bash extern/build.sh --help` for all options (e.g. `--jobs N`, `--force`).

After a successful build the layout under `extern/` is:

```
extern/
  installed/                         ← cpr, influxdb-cxx, cassandra driver
  QDMI-Devices/build/
    src/databases/dcdb/              ← libdcdb_device.so + headers
    _deps/qdmi-src/include/          ← QDMI core headers
    _deps/dcdb-src/lib/include/      ← DCDB headers
    _deps/dcdb-build/lib/            ← libdcdb.so
```

### Step 3 — Export environment variables {#env}

All library paths are computed automatically relative to the repository root:

```bash
source build/export_qdmi_dcdb_deps.sh
```

## Building the Daemon {#building}

With the environment variables set, configure and build:

```bash
cmake -S . -B build
cmake --build build -j
```

The daemon binary is placed at `build/mqss_dcdb_daemon`.
