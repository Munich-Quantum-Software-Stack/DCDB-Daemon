cmake_minimum_required(VERSION 3.16)

project(influxdb-example)

find_package(InfluxDB)

add_executable(influxdb-exp influxdb-write.cpp)
target_link_libraries(influxdb-exp PRIVATE InfluxData::InfluxDB)

