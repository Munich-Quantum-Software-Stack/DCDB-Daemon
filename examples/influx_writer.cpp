#include "influx_writer.h"

#include <influxdb-cxx/Point.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

// ---------------------------------------------------------------
// Internal helper: parse ISO-8601 timestamp → nanoseconds
// Supports both '%Y-%m-%dT%H:%M:%S.%f' and '%Y-%m-%dT%H:%M:%S'
// ---------------------------------------------------------------
static long long parse_timestamp_ns(const std::string& timestamp)
{
    std::tm tm{};
    long long microseconds = 0;

    auto dot_pos = timestamp.find('.');
    if (dot_pos != std::string::npos) {
        std::string ts_main = timestamp.substr(0, dot_pos);
        std::string ts_frac = timestamp.substr(dot_pos + 1);

        // Truncate or pad fractional part to exactly 6 digits (microseconds)
        if (ts_frac.size() > 6) ts_frac = ts_frac.substr(0, 6);
        while (ts_frac.size() < 6) ts_frac += '0';

        std::istringstream ss(ts_main);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        if (ss.fail())
            throw std::runtime_error("Failed to parse timestamp: " + ts_main);

        microseconds = std::stoll(ts_frac);
    } else {
        std::istringstream ss(timestamp);
        ss >> std::get_time(&tm, "%Y-%m-%dT%H:%M:%S");
        if (ss.fail())
            throw std::runtime_error("Failed to parse timestamp: " + timestamp);
    }

    // Convert struct tm → epoch seconds, treating input as UTC
    tm.tm_isdst = -1;
#ifdef _WIN32
    std::time_t epoch = _mkgmtime(&tm);
#else
    std::time_t epoch = timegm(&tm);  // POSIX (Linux / macOS)
#endif

    return static_cast<long long>(epoch) * 1'000'000'000LL
           + microseconds * 1'000LL;
}

// ---------------------------------------------------------------
// parse_data
// ---------------------------------------------------------------
std::vector<DataEntry> parse_data(const std::vector<std::string>& lines)
{
    std::vector<DataEntry> data;

    for (const auto& line : lines) {
        if (line.empty()) continue;

        // Split on ','
        std::vector<std::string> parts;
        {
            std::istringstream ss(line);
            std::string token;
            while (std::getline(ss, token, ','))
                parts.push_back(token);
        }

        if (parts.size() != 3) continue;

        const std::string& timestamp = parts[1];
        const std::string& raw_value = parts[2];

        if (timestamp == "Time") continue;  // skip header row

        try {
            long long ns  = parse_timestamp_ns(timestamp);
            double    val = std::stod(raw_value);
            data.push_back({ns, val});
        } catch (const std::exception& e) {
            std::cerr << "Error parsing line '" << line << "': "
                      << e.what() << '\n';
        }
    }

    return data;
}

// ---------------------------------------------------------------
// send_to_influxdb
// ---------------------------------------------------------------
void send_to_influxdb(
    influxdb::InfluxDB&           db,
    const std::vector<DataEntry>& data,
    const std::string&            data_source,
    double                        divisor,
    const std::string&            measurement,
    const std::string&            field_name)
{
    if (data.empty()) return;

    for (const auto& entry : data) {
        auto tp = std::chrono::time_point<std::chrono::system_clock>(
            std::chrono::nanoseconds(entry.time_ns));

        db.write(
            influxdb::Point{measurement}
                .addTag  ("source", data_source)
                .addTag  ("field",  field_name)
                .addField(field_name, entry.value / divisor)
                .setTimestamp(tp));
    }

    // Flush any buffered points (no-op when batching is disabled)
    db.flushBatch();

    std::cout << "Successfully wrote " << data.size()
              << " points to InfluxDB for " << data_source << '\n';
}
