# ------------------------------------------------------------------------------
# Copyright 2024 Munich Quantum Software Stack Project
#
# Licensed under the Apache License, Version 2.0 with LLVM Exceptions (the
# "License"); you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
# https://github.com/Munich-Quantum-Software-Stack/
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
# WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied. See the
# License for the specific language governing permissions and limitations under
# the License.
#
# SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
# ------------------------------------------------------------------------------

# ------------------------------------------------------------------------------
# InfluxDBClient.cmake
# ------------------------------------------------------------------------------
set(INFLUXDB_CLIENT_ROOT "/home/admin/shared/DCDB-Daemon/utils/influxdb-c-client/influxdb-cxx/install")
set(CMAKE_PREFIX_PATH "${INFLUXDB_CLIENT_ROOT};${CMAKE_PREFIX_PATH}")

# ------------------------------------------------------------------------------
# InfluxDBClient headers and libs
# ------------------------------------------------------------------------------
find_path(INFLUXDB_CLIENT_INCLUDE
    NAMES InfluxDB/InfluxDBFactory.h
    PATHS ${INFLUXDB_CLIENT_ROOT}/include
)
find_library(INFLUXDB_CLIENT_LIBRARY
    NAMES InfluxDB
    PATHS ${INFLUXDB_CLIENT_ROOT}/lib
)

# ------------------------------------------------------------------------------
# Check if found
# ------------------------------------------------------------------------------
include(FindPackageHandleStandardArgs)
find_package_handle_standard_args(InfluxDB
    REQUIRED_VARS INFLUXDB_CLIENT_INCLUDE INFLUXDB_CLIENT_LIBRARY
)

if(INFLUXDB_FOUND)
    set(INFLUXDB_CLIENT_INCLUDE_DIR ${INFLUXDB_CLIENT_INCLUDE})
    set(INFLUXDB_CLIENT_LIBRARY_DIR ${INFLUXDB_CLIENT_LIBRARY})
endif()

mark_as_advanced(INFLUXDB_CLIENT_INCLUDE_DIR INFLUXDB_CLIENT_LIBRARY_DIR)
message(STATUS "INFLUXDB_CLIENT_ROOT        = ${INFLUXDB_CLIENT_ROOT}")
message(STATUS "INFLUXDB_CLIENT_INCLUDE_DIR = ${INFLUXDB_CLIENT_INCLUDE_DIR}")
message(STATUS "INFLUXDB_CLIENT_LIBRARY_DIR = ${INFLUXDB_CLIENT_LIBRARY_DIR}")
