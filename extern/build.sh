#!/usr/bin/env bash
# ------------------------------------------------------------------------------
# Build and install DCDB-Daemon dependencies into extern/installed/
# Prerequisite: run clone.sh first.
#
# Usage: bash build.sh [OPTIONS]
#
# Options:
#   --jobs N              Parallel make jobs (default: all CPU cores)
#   --force               Rebuild all packages even if already installed
#   --with-qdmi-devices   Also build QDMI-Devices (requires QDMI-Devices to
#                         have been cloned and DCDB to be available)
#   --with-dcdb           Let QDMI-Devices fetch and build DCDB automatically
#                         via its FetchContent mechanism (long build; only used
#                         when --with-qdmi-devices is set)
#   --dcdb-root PATH      Path to a pre-installed DCDB tree; expected layout:
#                           <PATH>/install/lib/libdcdb.so
#                           <PATH>/install/include/        (dcdb/ headers)
#                           <PATH>/dcdb/common/include/    (common headers)
#                         Overrides --with-dcdb when both are given.
#                         Defaults to DCDB_ROOT env var if set.
#   --with-tests          Build QDMI-Devices backend tests (off by default).
#                         Enables BUILD_BACKEND_TESTS=ON for QDMI-Devices.
#
# Build order: cpr → influxdb-cxx → cpp-driver [→ QDMI-Devices]
# ------------------------------------------------------------------------------
set -euo pipefail

EXTERN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
INSTALL_DIR="$EXTERN_DIR/installed"

echo "Extern build script"
echo "  EXTERN_DIR  = $EXTERN_DIR"
echo "  INSTALL_DIR = $INSTALL_DIR"

JOBS=1
FORCE=false
WITH_QDMI_DEVICES=false
WITH_DCDB=false
WITH_TESTS=false
DCDB_ROOT="${DCDB_ROOT:-}"

while [[ $# -gt 0 ]]; do
    case "$1" in
        --jobs)              JOBS="$2";       shift 2 ;;
        --force)             FORCE=true;      shift   ;;
        --with-qdmi-devices) WITH_QDMI_DEVICES=true; shift ;;
        --with-dcdb)         WITH_DCDB=true;  shift   ;;
        --with-tests)        WITH_TESTS=true; shift   ;;
        --dcdb-root)         DCDB_ROOT="$2";  shift 2 ;;
        --help|-h)
            sed -n '/^# Usage/,/^# ----/p' "$0" | sed 's/^# \?//'
            exit 0 ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

mkdir -p "$INSTALL_DIR"

# Check whether a sentinel file already exists in installed/ to skip a rebuild.
# Usage: is_installed <relative-path-under-installed>
is_installed() {
    [[ "$FORCE" == false ]] && [[ -e "$INSTALL_DIR/$1" ]]
}

build_cmake() {
    local name="$1"
    local src="$2"
    local sentinel="$3"   # file to check in installed/ (e.g. lib/libcpr.so)
    shift 3

    if is_installed "$sentinel"; then
        echo ""
        echo "[skip]   $name already installed ($INSTALL_DIR/$sentinel exists)"
        echo "         Run with --force to rebuild."
        return 0
    fi

    echo ""
    echo "========================================"
    echo " Building: $name"
    echo "========================================"

    local build_dir="$src/build"
    mkdir -p "$build_dir"
    cmake -S "$src" -B "$build_dir" \
        -DCMAKE_BUILD_TYPE=Release \
        -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
        -DCMAKE_CXX_STANDARD=17 \
        "$@"
    cmake --build "$build_dir" --parallel "$JOBS"
    cmake --install "$build_dir"
    echo "[done]   $name installed to $INSTALL_DIR"
}

# ------------------------------------------------------------------------------
# 1. cpr  — C++ HTTP client
#    Use the system-provided libcurl to avoid a nested curl build.
# ------------------------------------------------------------------------------
build_cmake "cpr" "$EXTERN_DIR/cpr" "lib/libcpr.so" \
    -DBUILD_SHARED_LIBS=ON \
    -DCPR_USE_SYSTEM_CURL=ON \
    -DCPR_BUILD_TESTS=OFF \
    -DCPR_ENABLE_SSL=ON \
    -DCPR_FORCE_OPENSSL_BACKEND=ON

# ------------------------------------------------------------------------------
# 2. influxdb-cxx  — C++ InfluxDB client (depends on cpr)
# ------------------------------------------------------------------------------
build_cmake "influxdb-cxx" "$EXTERN_DIR/influxdb-cxx" "lib/libInfluxDB.so" \
    -DCMAKE_PREFIX_PATH="$INSTALL_DIR" \
    -DINFLUXCXX_TESTING=OFF \
    -DINFLUXCXX_WITH_BOOST=OFF \
    -DINFLUXCXX_WERROR=OFF

# ------------------------------------------------------------------------------
# 3. cpp-driver — ScyllaDB C/C++ Cassandra driver (optional, for QDMI-Devices)
#    DCDB links against libscylla-cpp-driver specifically (ScyllaDB's fork adds
#    cass_cluster_set_local_port_range and other extensions). Depends on libuv
#    and libssl.
# ------------------------------------------------------------------------------
if $WITH_QDMI_DEVICES; then
    build_cmake "cpp-driver" "$EXTERN_DIR/cpp-driver" "lib/libscylla-cpp-driver.so" \
        -DCASS_BUILD_SHARED=ON \
        -DCASS_BUILD_STATIC=OFF \
        -DCASS_BUILD_TESTS=OFF \
        -DCASS_BUILD_EXAMPLES=OFF \
        -DCASS_BUILD_DOCS=OFF
fi

# ------------------------------------------------------------------------------
# 4. QDMI-Devices — dcdb quantum-device backend (optional)
# ------------------------------------------------------------------------------
if $WITH_QDMI_DEVICES; then
    QDMI_SRC="$EXTERN_DIR/QDMI-Devices"
    QDMI_BUILD="$QDMI_SRC/build"

    if [[ ! -d "$QDMI_SRC/.git" ]]; then
        echo ""
        echo "ERROR: $QDMI_SRC is not a git repository."
        echo "       Run: bash clone.sh --with-qdmi-devices"
        exit 1
    fi

    if is_installed "lib/libdcdb_device.so"; then
        echo ""
        echo "[skip]   QDMI-Devices already installed ($INSTALL_DIR/lib/libdcdb_device.so exists)"
        echo "         Run with --force to rebuild."
    else
        echo ""
        echo "========================================"
        echo " Building: QDMI-Devices (dcdb module)"
        echo "========================================"

        # Decide how to supply DCDB
        DCDB_CMAKE_ARGS=()
        if [[ -n "$DCDB_ROOT" ]]; then
            # Pre-installed DCDB: pass paths directly and skip the FetchContent build
            DCDB_LIB="$DCDB_ROOT/install/lib/libdcdb.so"
            DCDB_INC="$DCDB_ROOT/install/include"
            DCDB_COMMON_INC="$DCDB_ROOT/dcdb/common/include"

            if [[ ! -f "$DCDB_LIB" ]]; then
                echo "ERROR: DCDB library not found at $DCDB_LIB"
                echo "       Check --dcdb-root / DCDB_ROOT and ensure DCDB is built."
                exit 1
            fi

            echo "  Using pre-installed DCDB at: $DCDB_ROOT"
            DCDB_CMAKE_ARGS=(
                -DBUILD_BACKEND_DCDB=OFF
                "-DDCDB_LIBRARY=$DCDB_LIB"
                "-DDCDB_INCLUDE_DIRS=$DCDB_INC;$DCDB_COMMON_INC"
            )
        elif $WITH_DCDB; then
            # Let QDMI-Devices fetch and build DCDB in-tree (long build)
            echo "  DCDB will be fetched and built automatically (this takes a while)."
            DCDB_CMAKE_ARGS=(-DBUILD_BACKEND_DCDB=ON)
        else
            echo "ERROR: QDMI-Devices needs DCDB. Either:"
            echo "  1. Pass --dcdb-root <path>  (recommended; DCDB pre-installed)"
            echo "  2. Pass --with-dcdb         (fetch + build DCDB automatically)"
            exit 1
        fi

        # Export compiler search paths so the external Make project (DCDB) and
        # cmake-based targets both find cassandra.h / libcassandra.so in
        # INSTALL_DIR without needing a system-wide install.
        export CPATH="$INSTALL_DIR/include${CPATH:+:$CPATH}"
        export LIBRARY_PATH="$INSTALL_DIR/lib${LIBRARY_PATH:+:$LIBRARY_PATH}"
        export LD_LIBRARY_PATH="$INSTALL_DIR/lib${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"

        mkdir -p "$QDMI_BUILD"
        BUILD_TESTS_FLAG="OFF"
        if $WITH_TESTS; then BUILD_TESTS_FLAG="ON"; fi

        cmake -S "$QDMI_SRC" -B "$QDMI_BUILD" \
            -DCMAKE_BUILD_TYPE=Release \
            -DCMAKE_INSTALL_PREFIX="$INSTALL_DIR" \
            -DCMAKE_PREFIX_PATH="$INSTALL_DIR" \
            -DCMAKE_CXX_FLAGS="-I$INSTALL_DIR/include" \
            -DCMAKE_SHARED_LINKER_FLAGS="-L$INSTALL_DIR/lib" \
            -DCMAKE_CXX_STANDARD=17 \
            -DBUILD_MODULES=dcdb \
            -DBUILD_BACKEND_TESTS="$BUILD_TESTS_FLAG" \
            -DBUILD_DOCUMENTATION=OFF \
            "${DCDB_CMAKE_ARGS[@]}"
        cmake --build "$QDMI_BUILD" -- -j"$JOBS"
        cmake --install "$QDMI_BUILD"

        # The new MQSS-QDMI-Devices-Suite branch does not copy internal headers
        # (sensors.h, session.h) to the build include dir — copy them into the
        # dcdb_qdmi/ subdirectory alongside device.h and types.h.
        QDMI_DCDB_INC="$QDMI_BUILD/src/databases/dcdb/include/dcdb_qdmi"
        for hdr in sensors.h session.h; do
            src_hdr="$QDMI_SRC/src/databases/dcdb/$hdr"
            if [[ -f "$src_hdr" ]] && [[ ! -f "$QDMI_DCDB_INC/$hdr" ]]; then
                cp "$src_hdr" "$QDMI_DCDB_INC/$hdr"
                echo "[copy]   $hdr → $QDMI_DCDB_INC/"
            fi
        done

        echo "[done]   QDMI-Devices installed to $INSTALL_DIR"
    fi
fi

# ------------------------------------------------------------------------------
# Summary
# ------------------------------------------------------------------------------
echo ""
echo "========================================"
echo " Installed packages in: $INSTALL_DIR"
echo "   cpr, influxdb-cxx, cassandra driver"
echo ""
if $WITH_QDMI_DEVICES; then
    QDMI_BUILD="$EXTERN_DIR/QDMI-Devices/build"
    echo " QDMI-DCDB device (not installed, stays in build tree):"
    echo "   Library : $QDMI_BUILD/src/databases/dcdb/libdcdb_device.so"
    echo "   Headers : $QDMI_BUILD/src/databases/dcdb/include/"
    echo "   QDMI inc: $QDMI_BUILD/_deps/qdmi-src/include/"
    echo "   DCDB inc: $QDMI_BUILD/_deps/dcdb-src/lib/include/"
    echo "   DCDB lib: $QDMI_BUILD/_deps/dcdb-src/lib/"
    echo ""
fi
echo " Before building DCDB-Daemon, source:"
echo "   source build/export_qdmi_dcdb_deps.sh"
echo "========================================"
