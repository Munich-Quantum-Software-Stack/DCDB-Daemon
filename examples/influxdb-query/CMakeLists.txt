cmake_minimum_required(VERSION 3.16)

project(influxdb-query)

find_package(InfluxDB)

add_executable(influxdb-query influxdb-query.cpp)
target_link_libraries(influxdb-query PRIVATE InfluxData::InfluxDB)

