#include "daemon.hpp"
#include "arguments.hpp"
#include "database.hpp"

#include <iostream>
#include <cstdlib>
#include <chrono>

int main(int argc, char **argv)
{
    // Load the configuration from environment variables
    Config cfg;
    cfg.daemon_id = 101;
    cfg.daemon_name = "mqss-dcdb-daemon";
    cfg.dcdb_host_server = "host.docker.internal:9042";

    // Connect to InfluxDB
    auto db = influxdb::InfluxDBFactory::Get(INFLUX_URL);
    if (!db) {
        std::cerr << "Failed to connect to InfluxDB\n";
        return 1;
    }
    std::cout << "Connected to InfluxDB: " << INFLUX_URL << '\n';

    // Parse the input arguments
    Arguments args = parse_args(argc, argv);
    const int sensor_dev_id = std::stoi(args.sensor_id);

    // The sensor device id to query
    if (sensor_dev_id < 0) {
        std::cerr << "Fatal: Invalid sensor device id: " << sensor_dev_id << std::endl;
        return 1;
    }

    if (args.minutes_ago.has_value()) {
        std::cout << "Querying sensor data from the last " << *args.minutes_ago << " minutes." << std::endl;
    } else {
        std::string start_timestamp = args.start_time.value();
        std::string end_timestamp = args.end_time.value();
        std::cout << "Querying sensor data from " << start_timestamp << " to " << end_timestamp << "." << std::endl;
    }

    try {
        Daemon dm(cfg);
        std::cout << "------------------------------------------" << std::endl;
        std::cout << "MQSS QDMI-based DCDB Daemon.              " << std::endl;
        std::cout << "------------------------------------------" << std::endl;

        size_t size = 0;
        static DCDB_QDMI_Device_Session session;
        DCDB_QDMI_Device_TelemetrySensor_Query query;

        // Allocate and intialize the QDMI session
        DCDB_QDMI_device_initialize();
        DCDB_QDMI_device_session_alloc(&session);
        if (session == nullptr) {
            std::cerr << "Fatal: Error when allocating QDMI Device session for DCDB" << std::endl;
            return 1;
        }
        DCDB_QDMI_device_session_set_parameter(session, QDMI_DEVICE_SESSION_PARAMETER_BASEURL,
                                                cfg.dcdb_host_server.size(),
                                                cfg.dcdb_host_server.c_str());
        DCDB_QDMI_device_session_init(session);
        DCDB_QDMI_device_session_query_device_property(session, QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, 
                                                        0, nullptr, &size);
        std::vector<DCDB_QDMI_TelemetrySensor> envs(size/sizeof(DCDB_QDMI_TelemetrySensor));
        DCDB_QDMI_device_session_query_device_property(session, QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, 
                                                        size, static_cast<void *>(envs.data()), nullptr);
        DCDB_QDMI_device_session_create_device_telemetrysensor_query(session, &query);

        std::cout << "------------------------------------------" << std::endl;
        std::cout << "QDMI-based DCDB Device is initialized successfully." << std::endl;
        std::cout << "MQSS QDMI-based DCDB Daemon initialized successfully." << std::endl;
        std::cout << "MQSS DCDB Daemon ID: "   << cfg.daemon_id << std::endl;
        std::cout << "MQSS DCDB Daemon Name: " << cfg.daemon_name << std::endl;
        std::cout << "DCDB Host Server: " << cfg.dcdb_host_server << std::endl;
        std::cout << "------------------------------------------" << std::endl;

        // Timestamp to get the sensor data
        //  + Query from now
        //  + And from 10 minutes ago
        int duration_minutes_ago = 10;
        int duration_hours_ago = 120;
        auto time_now = std::chrono::system_clock::now();
        auto time_start = time_now - std::chrono::minutes(duration_hours_ago + duration_minutes_ago);
        auto time_end = time_now - std::chrono::minutes(duration_hours_ago);

        std::time_t time_start_value = std::chrono::system_clock::to_time_t(time_start);
        std::time_t time_end_value = std::chrono::system_clock::to_time_t(time_end);

        std::string ctime_format_start = std::ctime(&time_start_value);
        std::string ctime_format_end = std::ctime(&time_end_value);
        std::cout << "------------------------------------------" << std::endl;
        std::cout << "Query Time Window:  " << std::endl;
        std::cout << " + Start Timestamp: " << ctime_format_start;
        std::cout << " + End Timestamp:   " << ctime_format_end;
        std::cout << "------------------------------------------" << std::endl;

        uint64_t endtime_unixformat = static_cast<uint64_t>(time_end_value);
        uint64_t startime_unixformat = static_cast<uint64_t>(time_start_value);

        /*
        * Convert to unix epoch seconds format, for example,
        *  + 1745644603 --> 2025-04-26 07:16:43
        *  + 1745817403 --> 2025-04-28 07:16:43
        * std::string start_timestamp = "2025-06-26 07:16:10";
        * std::string end_timestamp   = "2025-06-26 07:26:10";
        * uint64_t sta_ts = timestamp2unix_epoch_seconds(start_timestamp);
        * uint64_t end_ts = timestamp2unix_epoch_seconds(end_timestamp);
        */

        for (const auto& src : DATA_SOURCES) {
            if (src.sensor_id == sensor_dev_id) {
                std::cout << "\nSensor ID: "   << src.sensor_id
                          << "\nDivisor:     " << src.divisor
                          << "\nMeasurement: " << src.measurement
                          << "\nField:       " << src.field_name << '\n';
                std::cout << "------------------------------------------" << std::endl;

                DCDB_QDMI_TelemetrySensor sensor_dev = envs.at(sensor_dev_id);

                // Check the sensor id name
                std::string sensor_name = check_sensor_device_id(session, sensor_dev);
                std::cout << "QDMI-based DCDB Query: " << sensor_name << std::endl;

                // Query the sensor data
                DCDBSensorQueryResult query_results = mqss_dcdb_query_sensor_data(session, query, sensor_dev, 
                    startime_unixformat, endtime_unixformat);
                
                const std::string sensor_path = sensor_name;
                write_to_influxdb(*db, query_results, sensor_path, src);
                
                // Print the query results
                // std::cout << "------------------------------------------" << std::endl;
                // std::cout << "Writing Sensor Data Values: " << query_results.values.size() << "\n";
                // for (auto value : query_results.values){
                //     std::cout << value << " ";
                // }
                // std::cout << std::endl;

                // std::cout << "Writing Timestamps: " << query_results.timestamps.size() << "\n";
                // for (auto timestamp : query_results.timestamps){
                //     // Timestamp is in nanoseconds, convert to seconds, then human-readable format
                //     auto datetime_str = unix_epoch_seconds2datetime(timestamp / 1'000'000'000ULL);
                //     std::cout << datetime_str << " (" << timestamp << ") \n";
                // }
                // std::cout << std::endl;
                std::cout << "------------------------------------------" << std::endl;
            }
        }

        // Tear down the session and clean up
        std::cout << "Cleaning up QDMI session and resources..." << std::endl;
        DCDB_QDMI_device_telemetrysensor_query_free(query);
        DCDB_QDMI_device_session_free(session);
        DCDB_QDMI_device_finalize();

        return 0;
    }
    catch (const std::exception &ex){
        std::cerr << "Fatal: " << ex.what() << std::endl;
        return 1;
    }
}