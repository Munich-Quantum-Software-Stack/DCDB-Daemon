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

cmake_minimum_required(VERSION 3.12.0)

project(example_influxdb_core3 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)

find_package(CURL REQUIRED)
add_executable(example_influxdb_connect example_influxdb_connect.cpp)
target_link_libraries(example_influxdb_connect PRIVATE CURL::libcurl)