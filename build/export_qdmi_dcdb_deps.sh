#!/usr/bin/env bash
# Export env vars needed to build DCDB-Daemon against the locally built
# dependencies in extern/. Source this script before running cmake or make.
#
# Usage: source build/export_qdmi_dcdb_deps.sh

SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DAEMON_DIR="$(cd "$SCRIPT_DIR/.." && pwd)"
EXTERN_DIR="$DAEMON_DIR/extern"

INSTALL_DIR="$EXTERN_DIR/installed"
QDMI_BUILD="$EXTERN_DIR/QDMI-Devices/build"

# cpr + influxdb-cxx + cassandra driver — all installed into extern/installed
export CMAKE_PREFIX_PATH="$INSTALL_DIR${CMAKE_PREFIX_PATH:+:$CMAKE_PREFIX_PATH}"

# QDMI-DCDB device library and headers — not installed, lives in the build tree
export QDMI_DCDB_DEV_DIR="$QDMI_BUILD/src/databases/dcdb"
export QDMI_DCDB_DEV_INC_DIR="$QDMI_DCDB_DEV_DIR/include"
export QDMI_DCDB_DEV_LIB_DIR="$QDMI_DCDB_DEV_DIR"

# QDMI core headers — fetched via FetchContent inside QDMI-Devices build
export QDMI_INC_DIR="$QDMI_BUILD/_deps/qdmi-src/include"

# DCDB — fetched and built via FetchContent inside QDMI-Devices build.
# DCDB uses its own Makefile (ExternalProject), so outputs stay in dcdb-src/lib/.
DCDB_DEPS="$QDMI_BUILD/_deps"
export DCDB_INC_DIR="$DCDB_DEPS/dcdb-src/lib/include"
export DCDB_LIB_DIR="$DCDB_DEPS/dcdb-src/lib"
export DCDB_COMMON_DIR="$DCDB_DEPS/dcdb-src/common"
export DCDB_SOURCE_LOCATION="$DCDB_DEPS/dcdb-src"

export CPATH="$DCDB_INC_DIR:$DCDB_COMMON_DIR/include:$QDMI_DCDB_DEV_INC_DIR:$QDMI_INC_DIR:$INSTALL_DIR/include${CPATH:+:$CPATH}"
export INCLUDE="$DCDB_INC_DIR:$DCDB_COMMON_DIR/include:$QDMI_DCDB_DEV_INC_DIR:$QDMI_INC_DIR:$INSTALL_DIR/include${INCLUDE:+:$INCLUDE}"
export LD_LIBRARY_PATH="$DCDB_LIB_DIR:$QDMI_DCDB_DEV_LIB_DIR:$INSTALL_DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export LIBRARY_PATH="$DCDB_LIB_DIR:$QDMI_DCDB_DEV_LIB_DIR:$INSTALL_DIR/lib${LIBRARY_PATH:+:$LIBRARY_PATH}"

echo "---------------------------------------------------"
echo "Exported DCDB, QDMI, QDMI_DCDB Device env vars."
echo ""
echo "  INSTALL_DIR           = $INSTALL_DIR"
echo ""
echo "  DCDB_INC_DIR          = $DCDB_INC_DIR"
echo "  DCDB_LIB_DIR          = $DCDB_LIB_DIR"
echo "  DCDB_COMMON_DIR       = $DCDB_COMMON_DIR"
echo ""
echo "  QDMI_INC_DIR          = $QDMI_INC_DIR"
echo "  QDMI_DCDB_DEV_INC_DIR = $QDMI_DCDB_DEV_INC_DIR"
echo "  QDMI_DCDB_DEV_LIB_DIR = $QDMI_DCDB_DEV_LIB_DIR"
echo "---------------------------------------------------"
