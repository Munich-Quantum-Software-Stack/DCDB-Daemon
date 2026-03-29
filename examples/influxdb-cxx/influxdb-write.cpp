#include "influxdb-write.hpp"
#include <InfluxDB/InfluxDBFactory.h>
#include <InfluxDB/Point.h>

#include <chrono>
#include <ctime>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <stdexcept>

// ---------------------------------------------------------------
// InfluxDB configuration (mirrored from Python)
// ---------------------------------------------------------------
static const std::string INFLUX_URL =
    "http://account:credentials"
    "@portal-telemetry.de:8086?db=database_name";

// ---------------------------------------------------------------
// Data sources: { path, divisor, measurement, field_name }
// ---------------------------------------------------------------
static const std::vector<DataSource> DATA_SOURCES = {
    { "/qic/warmlab/humidity",       1000.0, "environmental", "humidity"    },
    { "/qic/warmlab/temperature",    1000.0, "environmental", "temperature" },
    { "/qic/warmlab/pressure",        100.0, "environmental", "pressure"    },
    { "/qic/warmlab/light",             1.0, "environmental", "light"       },
    { "/qic/warmlab/loudness",          1.0, "environmental", "loudness"    },
    { "/qic/warmlab/movement",          1.0, "environmental", "movement"    },
};

// ---------------------------------------------------------------
// Fake data generator — mimics the Python example output
// Replace with real dcdbquery calls if needed
// ---------------------------------------------------------------
std::vector<DataEntry> fake_data(const std::string& path)
{
    // Base timestamps and values modelled after the temperature example
    // Each source gets slightly varied values so they look distinct
    auto now = std::chrono::system_clock::now();
    auto start = now - std::chrono::hours(2);

    std::vector<long long> base_times;
    for (int i = 0; i < 5; ++i) {
        auto tp = start + std::chrono::minutes(i);
        auto ns = std::chrono::duration_cast<std::chrono::nanoseconds>(
                      tp.time_since_epoch()).count();
        base_times.push_back(ns);
    }

    // Pick a base raw value per sensor path
    double base_raw = 19713.0;
    if (path.find("humidity")         != std::string::npos) base_raw = 45200.0;
    else if (path.find("temperature") != std::string::npos) base_raw = 19713.0;
    else if (path.find("pressure")    != std::string::npos) base_raw = 101325.0;
    else if (path.find("light")       != std::string::npos) base_raw = 320.0;
    else if (path.find("loudness")    != std::string::npos) base_raw = 55.0;
    else if (path.find("movement")    != std::string::npos) base_raw = 0.0;

    std::vector<DataEntry> entries;
    for (size_t i = 0; i < base_times.size(); ++i)
        entries.push_back({ base_times[i], base_raw + static_cast<double>(i) });

    return entries;
}

// ---------------------------------------------------------------
// parse_timestamp_ns  (unchanged from your colleague's version)
// ---------------------------------------------------------------
static long long parse_timestamp_ns(const std::string& timestamp)
{
    std::tm tm{};
    long long microseconds = 0;

    auto dot_pos = timestamp.find('.');
    if (dot_pos != std::string::npos) {
        std::string ts_main = timestamp.substr(0, dot_pos);
        std::string ts_frac = timestamp.substr(dot_pos + 1);
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

    tm.tm_isdst = -1;
#ifdef _WIN32
    std::time_t epoch = _mkgmtime(&tm);
#else
    std::time_t epoch = timegm(&tm);
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
        if (timestamp == "Time") continue;
        try {
            data.push_back({ parse_timestamp_ns(timestamp), std::stod(raw_value) });
        } catch (const std::exception& e) {
            std::cerr << "Error parsing line '" << line << "': " << e.what() << '\n';
        }
    }
    return data;
}

// ---------------------------------------------------------------
// send_to_influxdb  (unchanged)
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
        // Convert back to time_t to verify
        auto tp = std::chrono::time_point<std::chrono::system_clock, std::chrono::nanoseconds>(std::chrono::nanoseconds(entry.time_ns));
        time_t t = std::chrono::system_clock::to_time_t(tp);

        std::cout << "Writing to InfluxDB: "
                  << "Measurement: " << measurement
                  << ", Field: " << field_name
                  << ", Value: " << (entry.value / divisor)
                  << ", Timestamp: " << entry.time_ns << " ns"
                  << "--> " << std::ctime(&t) << "\n";
        std::cout << "Check tp: " << tp.time_since_epoch().count() << "\n";

        // db.write(
        //     influxdb::Point{measurement}
        //         .addTag  ("source", data_source)
        //         .addTag  ("field",  field_name)
        //         .addField(field_name, entry.value / divisor)
        //         .setTimestamp(tp));
        // std::cout << "Write successful.\n";
    }
}

// ---------------------------------------------------------------
// main
// ---------------------------------------------------------------
int main()
{
    // Connect to InfluxDB
    auto db = influxdb::InfluxDBFactory::Get(INFLUX_URL);
    if (!db) {
        std::cerr << "Failed to connect to InfluxDB\n";
        return 1;
    }
    std::cout << "Connected to InfluxDB: " << INFLUX_URL << '\n';

    for (const auto& src : DATA_SOURCES) {
        std::cout << "\nData source: " << src.path
                  << "\nDivisor:     " << src.divisor
                  << "\nMeasurement: " << src.measurement
                  << "\nField:       " << src.field_name << '\n';

        // Replace fake_data() with a real dcdbquery call if available
        auto data = fake_data(src.path);
        if (data.empty()) {
            std::cerr << "No data for " << src.path << ", skipping.\n";
            continue;
        }
        // std::cout << "Parsed " << data.size() << " data entries for " << src.path << ".\n";
        // std::cout << "First entry: timestamp=" << data.front().time_ns
        //           << " ns, value=" << data.front().value << "\n";

        try {
            send_to_influxdb(*db, data, src.path,
                             src.divisor, src.measurement, src.field_name);
        } catch (const std::exception& e) {
            std::cerr << "Error writing " << src.path << ": " << e.what() << '\n';
        }
    }

    return 0;
}