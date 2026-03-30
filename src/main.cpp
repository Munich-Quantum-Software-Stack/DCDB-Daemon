#include "daemon.hpp"
#include "arguments.hpp"
#include "database.hpp"

#include <iostream>
#include <cstdlib>
#include <chrono>
#include <atomic>
#include <csignal>
#include <thread>

// Global flag — set to false when Ctrl+C is pressed
static std::atomic<bool> running{true};

// Signal handler — called on Ctrl+C (SIGINT) or kill (SIGTERM)
static void signal_handler(int signal) {
    std::cout << "\nReceived signal " << signal << ", stopping...\n";
    running = false;
}

// Add alongside your SIGINT handler at the top of main.cpp
static void crash_handler(int signal) {
    std::cerr << "\nCaught signal " << signal << " (SIGSEGV/SIGABRT), "
              << "attempting to continue...\n";
    // We can't safely continue after SIGSEGV, but we can at least log it
    // The safest option is to just exit cleanly
    running = false;
}

int main(int argc, char **argv)
{
    // Register signal handlers
    std::signal(SIGINT,  signal_handler);  // Ctrl+C
    std::signal(SIGTERM, signal_handler);  // kill command
    std::cout << "Daemon running. Press Ctrl+C to stop.\n";

    // std::signal(SIGSEGV, crash_handler);  // segfault
    // std::signal(SIGABRT, crash_handler);  // abort (heap corruption)

    // --- Step 1: Parse and validate arguments first ---
    Arguments args = parse_args(argc, argv);
    const int sensor_dev_id = std::stoi(args.sensor_id);
    if (sensor_dev_id < 0) {
        std::cerr << "Fatal: Invalid sensor device id: " << sensor_dev_id << '\n';
        return 1;
    }

    // --- Step 2: Connect to InfluxDB ---
    auto db = influxdb::InfluxDBFactory::Get(INFLUX_URL);
    if (!db) {
        std::cerr << "Fatal: could not connect to InfluxDB\n";
        return 1;
    }
    std::cout << "Connected to InfluxDB: " << INFLUX_URL << '\n';

    // --- Step 3: Initialize DCDB Config ---
    Config cfg;
    cfg.daemon_id        = 101;
    cfg.daemon_name      = "mqss-dcdb-daemon";
    cfg.dcdb_host_server = "host.docker.internal:9042";

    try {
        Daemon dm(cfg);

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

        std::cout << "------------------------------------------\n"
                  << "QDMI-DCDB Daemon initialized. Daemon ID: " << cfg.daemon_id
                  << " | Host Server: " << cfg.dcdb_host_server << '\n'
                  << "Daemon running. Press Ctrl+C to stop.\n"
                  << "------------------------------------------\n";

        /*
        * Convert to unix epoch seconds format, for example,
        *  + 1745644603 --> 2025-04-26 07:16:43
        *  + 1745817403 --> 2025-04-28 07:16:43
        * std::string start_timestamp = "2025-06-26 07:16:10";
        * std::string end_timestamp   = "2025-06-26 07:26:10";
        * uint64_t sta_ts = timestamp2unix_epoch_seconds(start_timestamp);
        * uint64_t end_ts = timestamp2unix_epoch_seconds(end_timestamp);
        */

        /* 
        * The main loop of the daemon — runs until Ctrl+C is pressed. In a real implementation, 
        * we can add some sleep/delay here to avoid busy-waiting
        */

        // --- Step 4: Main loop ---
        while (running) {
            auto loop_start = std::chrono::steady_clock::now();

            int duration_minutes = 10;
            int two_hours_ago = 120;
            auto time_now   = std::chrono::system_clock::now();
            auto time_start = time_now - std::chrono::minutes(two_hours_ago + duration_minutes);
            auto time_end = time_now - std::chrono::minutes(two_hours_ago);

            uint64_t sta_ts = static_cast<uint64_t>(std::chrono::system_clock::to_time_t(time_start));
            uint64_t end_ts = static_cast<uint64_t>(std::chrono::system_clock::to_time_t(time_end));

            std::time_t t_start = static_cast<std::time_t>(sta_ts);
            std::time_t t_end   = static_cast<std::time_t>(end_ts);
            std::string str_start = std::ctime(&t_start);
            std::string str_end   = std::ctime(&t_end);
            if (!str_start.empty() && str_start.back() == '\n') str_start.pop_back();
            if (!str_end.empty()   && str_end.back()   == '\n') str_end.pop_back();

            std::cout << "------------------------------------------\n"
                      << "Query Time Window:\n"
                      << " + Start: " << str_start << '\n'
                      << " + End:   " << str_end   << '\n'
                      << "------------------------------------------\n";

            // --- Query and write ---
            for (const auto& src : DATA_SOURCES) {

                // check between sensors too
                if (!running) break;

                try {
                    // Fresh query handle for each sensor
                    DCDB_QDMI_device_session_create_device_telemetrysensor_query(session, &query);

                // if (src.sensor_id == sensor_dev_id) {
                    std::cout << "\nSensor ID: "   << src.sensor_id
                              << "\nDivisor:     " << src.divisor
                              << "\nMeasurement: " << src.measurement
                              << "\nField:       " << src.field_name << '\n';
                    std::cout << "------------------------------------------" << std::endl;

                    DCDB_QDMI_TelemetrySensor sensor_dev = envs.at(sensor_dev_id);

                    // Check the sensor id name
                    std::string sensor_name = check_sensor_device_id(session, sensor_dev);

                    // Query the sensor data
                    DCDBSensorQueryResult query_results = mqss_dcdb_query_sensor_data(session, query, sensor_dev, sta_ts, end_ts);
                    
                    // Write to InfluxDB
                    const std::string sensor_path = sensor_name;
                    write_to_influxdb(*db, query_results, sensor_path, src);

                    // Explicitly clear results after writing
                    query_results.values.clear();
                    query_results.timestamps.clear();
                    query_results.values.shrink_to_fit();
                    query_results.timestamps.shrink_to_fit();

                    // Fresh query handle for each sensor
                    DCDB_QDMI_device_telemetrysensor_query_free(query);
                // }
                } catch (const std::exception& e) {
                    std::cerr << "Skipping sensor " << src.sensor_id
                              << " (" << src.field_name << "): " << e.what() << '\n';

                    // Try to recover the query handle for the next sensor
                    try {
                        DCDB_QDMI_device_telemetrysensor_query_free(query);
                        DCDB_QDMI_device_session_create_device_telemetrysensor_query(session, &query);
                    } catch (...) {
                        std::cerr << "Query handle recovery failed for sensor "
                                  << src.sensor_id << ", continuing...\n";
                    }

                    continue;  // skip to next sensor
                }
            }

            // --- Sleep for 5 minutes, but wake up early if stopped ---
            auto next_run = loop_start + std::chrono::minutes(5);
            while (running && std::chrono::steady_clock::now() < next_run) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
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