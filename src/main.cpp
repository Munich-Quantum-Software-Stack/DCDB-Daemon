#include "daemon.hpp"
#include "arguments.hpp"
#include "database.hpp"
#include "config.hpp"

#include <iostream>
#include <cstdlib>
#include <chrono>
#include <atomic>
#include <csignal>
#include <thread>
#include <cerrno>
#include <cstring>
#include <sys/wait.h>

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

    // --- Step 1b: Load JSON config ---
    AppConfig app_cfg;
    try {
        app_cfg = load_config(args.config_path);
    } catch (const std::exception& e) {
        std::cerr << "Fatal: " << e.what() << '\n';
        return 1;
    }
    std::cout << "Loaded config from: " << args.config_path << '\n'
              << "Active sensors: " << app_cfg.data_sources.size() << '\n';

    // --- Step 2: Connect to InfluxDB ---
    auto db = influxdb::InfluxDBFactory::Get(app_cfg.influx_url);
    if (!db) {
        std::cerr << "Fatal: could not connect to InfluxDB\n";
        return 1;
    }
    std::cout << "Connected to InfluxDB: " << app_cfg.influx_url << '\n';

    // --- Step 3: Initialize DCDB Config ---
    Config cfg;
    cfg.daemon_id        = 101;
    cfg.daemon_name      = "mqss-dcdb-daemon";
    cfg.dcdb_host_server = "host.docker.internal:9042";

    try {
        Daemon dm(cfg);

        size_t size = 0;
        static DCDB_QDMI_Device_Session session;

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

            int duration_minutes = 5;
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
            // Each sensor runs in a child process so that a library crash
            // (heap corruption → SIGABRT) only kills the child, not the daemon.
            for (const auto& src : app_cfg.data_sources) {

                if (!running) break;

                std::cout << "\nSensor ID: "   << src.sensor_id
                          << "\nDivisor:     " << src.divisor
                          << "\nMeasurement: " << src.measurement
                          << "\nField:       " << src.field_name << '\n';
                std::cout << "------------------------------------------" << std::endl;

                pid_t child_pid = fork();
                if (child_pid < 0) {
                    std::cerr << "fork() failed for sensor " << src.sensor_id
                              << ": " << std::strerror(errno) << " — skipping\n";
                    continue;
                }

                if (child_pid == 0) {
                    // ---- Child process ----
                    // Ignore Ctrl+C and SIGTERM — the parent handles shutdown and will
                    // SIGKILL us if needed. Without this, both parent and child would
                    // print the signal message and the child would never exit cleanly.
                    std::signal(SIGINT,  SIG_IGN);
                    std::signal(SIGTERM, SIG_IGN);

                    // Owns its own InfluxDB connection so the parent's socket is untouched.
                    // Uses _exit() to skip C++ destructors on shared resources.
                    try {
                        DCDB_QDMI_Device_TelemetrySensor_Query child_query;
                        auto child_db = influxdb::InfluxDBFactory::Get(app_cfg.influx_url);

                        DCDB_QDMI_device_session_create_device_telemetrysensor_query(session, &child_query);
                        DCDB_QDMI_TelemetrySensor sensor_dev = envs.at(src.sensor_id);
                        std::string sensor_name = check_sensor_device_id(session, sensor_dev);

                        DCDBSensorQueryResult query_results = mqss_dcdb_query_sensor_data(
                            session, child_query, sensor_dev, sta_ts, end_ts);

                        write_to_influxdb(*child_db, query_results, sensor_name, src);

                        DCDB_QDMI_device_telemetrysensor_query_free(child_query);
                    } catch (const std::exception& e) {
                        std::cerr << "Sensor " << src.sensor_id
                                  << " exception: " << e.what() << '\n';
                        _exit(1);
                    }
                    _exit(0);
                }

                // ---- Parent process: wait for child with timeout ----
                int status = 0;
                const int timeout_seconds = 10;
                bool timed_out = true;

                for (int i = 0; i < timeout_seconds; ++i) {
                    pid_t ret = waitpid(child_pid, &status, WNOHANG);
                    if (ret == child_pid) { timed_out = false; break; }
                    if (!running) break;  // Ctrl+C — let fall through to kill
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }

                if (timed_out || !running) {
                    kill(child_pid, SIGKILL);
                    waitpid(child_pid, &status, 0);
                    std::cerr << "Sensor " << src.sensor_id
                              << " (" << src.field_name << ") timed out after "
                              << timeout_seconds << "s — killed\n";
                } else if (WIFSIGNALED(status)) {
                    std::cerr << "Sensor " << src.sensor_id
                              << " (" << src.field_name << ")"
                              << " crashed (signal " << WTERMSIG(status) << ") — skipping\n";
                } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                    std::cerr << "Sensor " << src.sensor_id
                              << " (" << src.field_name << ") failed — skipping\n";
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
        DCDB_QDMI_device_session_free(session);
        DCDB_QDMI_device_finalize();

        return 0;
    }
    catch (const std::exception &ex){
        std::cerr << "Fatal: " << ex.what() << std::endl;
        return 1;
    }
}