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

# Set required cmake version
cmake_minimum_required(VERSION 3.19...4.0)
project(
  mqss_dcdb_daemon
  VERSION 1.0.0
  LANGUAGES CXX)

# Add C++ language support and enable building shared libs
enable_language(CXX)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_FLAGS "-g -DDEBUG")
set(BUILD_SHARED_LIBS ON)


# Specify the src files
file(GLOB MQSS_DCDB_DAEMON_SRC
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/src/*.hpp"
)

# Compile the executable
add_executable(mqss_dcdb_daemon ${MQSS_DCDB_DAEMON_SRC})

# Environment variable hints for DCDB and QDMI-Devices
set(ENV_DCDB_INC        $ENV{DCDB_INC_DIR})
set(ENV_DCDB_COMMON_INC $ENV{DCDB_COMMON_DIR}/include)
set(ENV_DCDB_LIB        $ENV{DCDB_LIB_DIR})

set(ENV_QDMI_INC        $ENV{QDMI_INC_DIR})
set(ENV_QDMI_DEV_INC    $ENV{QDMI_DCDB_DEV_INC_DIR})
set(ENV_QDMI_DEV_LIB    $ENV{QDMI_DCDB_DEV_LIB_DIR})

# message(STATUS "ENV_DCDB_LIB      = ${ENV_DCDB_LIB}")
# message(STATUS "ENV_QDMI_DEV_LIB  = ${ENV_QDMI_DEV_LIB}")

# Find the header files of dependencies
find_package(InfluxDB)
find_path(DCDB_INCLUDE_DIR NAMES dcdb/sensorconfig.h
  HINTS ${ENV_DCDB_INC} )
find_path(DCDB_COMMON_INCLUDE_DIR NAMES metadatastore.h
  HINTS ${ENV_DCDB_COMMON_INC} )
find_path(QDMI_INCLUDE_DIR NAMES qdmi/constants.h
  HINTS ${ENV_QDMI_INC} )
find_path(DCDB_QDMI_INCLUDE_DIR NAMES dcdb_qdmi/device.h
  HINTS ${ENV_QDMI_DEV_INC} )

message(STATUS "DCDB_INCLUDE_DIR        = ${DCDB_INCLUDE_DIR}")
message(STATUS "DCDB_COMMON_INCLUDE_DIR = ${DCDB_COMMON_INCLUDE_DIR}")
message(STATUS "QDMI_INCLUDE_DIR        = ${QDMI_INCLUDE_DIR}")
message(STATUS "DCDB_QDMI_INCLUDE_DIR   = ${DCDB_QDMI_INCLUDE_DIR}")

# Find the library files of dependencies
find_library(DCDB_LIBRARY NAMES dcdb dcdbclient dcdbcore
  HINTS ${ENV_DCDB_LIB} )
find_library(DCDB_QDMI_LIBRARY NAMES dcdb_qdmi dcdb_qdmi_devices dcdb_device
  HINTS ${ENV_QDMI_DEV_LIB} )
find_library(CASSANDRA_LIBRARY NAMES scylla-cpp-driver cassandra
  HINTS $ENV{QDMI_DCDB_DEV_LIB_DIR} $ENV{CMAKE_PREFIX_PATH}/lib )

message(STATUS "DCDB_LIBRARY      = ${DCDB_LIBRARY}")
message(STATUS "DCDB_QDMI_LIBRARY = ${DCDB_QDMI_LIBRARY}")

# nlohmann/json — header-only JSON parser fetched at configure time.
# include.zip extracts to <source_dir>/include/nlohmann/json.hpp
include(FetchContent)
FetchContent_Declare(
  nlohmann_json
  URL https://github.com/nlohmann/json/releases/download/v3.11.3/include.zip
  URL_HASH SHA256=a22461d13119ac5c78f205d3df1db13403e58ce1bb1794edc9313677313f4a9d
)
FetchContent_MakeAvailable(nlohmann_json)

# Interface target pointing at the include/ subdir inside the extracted zip
add_library(nlohmann_json_iface INTERFACE)
target_include_directories(nlohmann_json_iface INTERFACE
  ${nlohmann_json_SOURCE_DIR}/include
)

# Maybe thread lib on Linux
find_package(Threads REQUIRED)

# Apply the include directories (guard optional DCDB/QDMI deps against NOTFOUND)
foreach(_dir DCDB_INCLUDE_DIR DCDB_COMMON_INCLUDE_DIR QDMI_INCLUDE_DIR DCDB_QDMI_INCLUDE_DIR)
  if(${_dir})
    target_include_directories(mqss_dcdb_daemon PRIVATE "${${_dir}}")
  endif()
endforeach()

# Apply linking the dependencies (guard optional DCDB/QDMI deps against NOTFOUND)
foreach(_lib DCDB_LIBRARY DCDB_QDMI_LIBRARY CASSANDRA_LIBRARY)
  if(${_lib})
    target_link_libraries(mqss_dcdb_daemon PRIVATE "${${_lib}}")
  endif()
endforeach()
target_link_libraries(mqss_dcdb_daemon PRIVATE
  Threads::Threads
  InfluxData::InfluxDB
  nlohmann_json_iface
)

