#pragma once

#include <influxdb-cxx/InfluxDB.h>

#include <string>
#include <vector>

// ---------------------------------------------------------------
// DataEntry: holds a single parsed measurement sample
// ---------------------------------------------------------------
struct DataEntry {
    long long time_ns;  // Unix timestamp in nanoseconds
    double    value;
};

// ---------------------------------------------------------------
// parse_data
//
// Parses lines in the format "sensor,timestamp,value" where
// timestamp is ISO-8601 (with or without fractional seconds).
//
// Returns a vector of DataEntry with time in nanoseconds.
// ---------------------------------------------------------------
std::vector<DataEntry> parse_data(const std::vector<std::string>& lines);

// ---------------------------------------------------------------
// send_to_influxdb
//
// Writes parsed data points to InfluxDB, applying the given
// divisor to each value before writing.
//
// Parameters:
//   db          - open InfluxDB connection
//   data        - parsed data entries
//   data_source - tag value for "source" (e.g. "/qic/warmlab/...")
//   divisor     - scale factor applied to entry.value before writing
//   measurement - InfluxDB measurement name (e.g. "magnetometer")
//   field_name  - InfluxDB field key (e.g. "X")
// ---------------------------------------------------------------
void send_to_influxdb(
    influxdb::InfluxDB&           db,
    const std::vector<DataEntry>& data,
    const std::string&            data_source,
    double                        divisor,
    const std::string&            measurement,
    const std::string&            field_name);
