#!/usr/bin/env bash
# ------------------------------------------------------------------------------
# Clone all DCDB-Daemon dependencies into extern/
#
# Usage: bash clone.sh [--update] [--with-qdmi-devices]
#   --update             Pull latest changes if repos already exist
#   --with-qdmi-devices  Also clone QDMI-Devices (colleague's quantum device
#                        backends; requires DCDB to be pre-installed or built
#                        by build.sh via --with-dcdb)
# ------------------------------------------------------------------------------
set -euo pipefail

EXTERN_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"

UPDATE=false
WITH_QDMI_DEVICES=false

while [[ $# -gt 0 ]]; do
    case "$1" in
        --update)            UPDATE=true;            shift ;;
        --with-qdmi-devices) WITH_QDMI_DEVICES=true; shift ;;
        *) echo "Unknown option: $1"; exit 1 ;;
    esac
done

clone_or_update() {
    local name="$1"
    local url="$2"
    local ref="$3"       # branch or tag
    local ref_type="$4"  # "branch" or "tag"

    local dest="$EXTERN_DIR/$name"

    if [ -d "$dest/.git" ]; then
        if $UPDATE; then
            echo "[update] $name"
            git -C "$dest" fetch --tags origin
            git -C "$dest" checkout "$ref"
            if [ "$ref_type" = "branch" ]; then
                git -C "$dest" pull origin "$ref"
            fi
        else
            echo "[skip]   $name  (already cloned; use --update to refresh)"
        fi
    else
        echo "[clone]  $name  ($url @ $ref)"
        if [ "$ref_type" = "tag" ]; then
            git -c advice.detachedHead=false clone --depth 1 --branch "$ref" "$url" "$dest"
        else
            git clone --branch "$ref" "$url" "$dest"
        fi
    fi
}

# ------------------------------------------------------------------------------
# 1. cpr — C++ HTTP client (wraps libcurl)
# ------------------------------------------------------------------------------
clone_or_update \
    "cpr" \
    "https://github.com/libcpr/cpr.git" \
    "1.14.2" \
    "tag"

# ------------------------------------------------------------------------------
# 2. influxdb-cxx — C++ InfluxDB client (depends on cpr)
# ------------------------------------------------------------------------------
clone_or_update \
    "influxdb-cxx" \
    "https://github.com/offa/influxdb-cxx.git" \
    "v0.8.1" \
    "tag"

# ------------------------------------------------------------------------------
# 3. cpp-driver — ScyllaDB C/C++ driver (provides libscylla-cpp-driver.so)
#    Required by QDMI-Devices' DCDB backend. DCDB links against the ScyllaDB
#    fork specifically (adds cass_cluster_set_local_port_range and other
#    extensions absent from the DataStax driver).
# ------------------------------------------------------------------------------
if $WITH_QDMI_DEVICES; then
    clone_or_update \
        "cpp-driver" \
        "https://github.com/scylladb/cpp-driver.git" \
        "2.16.2-1" \
        "tag"
fi

# ------------------------------------------------------------------------------
# 4. QDMI-Devices — quantum device backends incl. the DCDB backend
#    (optional; implemented by a colleague)
# ------------------------------------------------------------------------------
if $WITH_QDMI_DEVICES; then
    clone_or_update \
        "QDMI-Devices" \
        "https://github.com/Munich-Quantum-Software-Stack/MQSS-QDMI-Devices-Suite.git" \
        "feature/add-query-loop-tests" \
        "branch"
fi

echo ""
echo "All sources are in: $EXTERN_DIR"
echo "Run build.sh next to compile and install them."

if ! $WITH_QDMI_DEVICES; then
    echo ""
    echo "  Note: QDMI-Devices was not cloned."
    echo "        Re-run with --with-qdmi-devices to include it."
    echo "        DCDB must be pre-installed separately (see build.sh --help)."
fi
