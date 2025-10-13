#include "daemon.hpp"
#include "arguments.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char **argv)
{
    // Parse the input arguments
    Arguments args = parse_args(argc, argv);
    const int sensor_dev_id = std::stoi(args.sensor_id);       // The sensor device id to query
    std::string start_timestamp = args.start_time.value();  // The start timestamp to query
    std::string end_timestamp   = args.end_time.value();    // The end timestamp to query

    Config cfg;
    cfg.daemon_id = 101;
    cfg.daemon_name = "mqss-dcdb-daemon";
    cfg.dcdb_host_server = "host.docker.internal:9042";

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
       
        DCDB_QDMI_TelemetrySensor sensor_dev = envs.at(sensor_dev_id); // The temperature in the Coldlab

        // Check the sensor id name
        std::string sensor_name = check_sensor_device_id(session, sensor_dev);
        std::cout << "QDMI-based DCDB Query: " << sensor_name << std::endl;

        // Timestamp to get the sensor data
        //  + Query from now
        //  + And from 10 minutes ago

        // int minutes_ago = 10;
        // auto time_now = std::chrono::system_clock::now();
        // auto time_pre = time_now - std::chrono::minutes(minutes_ago);
        // std::time_t time_now_value = std::chrono::system_clock::to_time_t(time_now);
        // std::time_t time_pre_value = std::chrono::system_clock::to_time_t(time_pre);
        // std::string ctime_format_now = std::ctime(&time_now_value);
        // std::string ctime_format_pre = std::ctime(&time_pre_value);
        // std::cout << "------------------------------------------" << std::endl;
        // std::cout << " + Time Now: "  << ctime_format_now;
        // std::cout << " + Time Prev -" << minutes_ago << " Minutes Ago: " << ctime_format_pre;
        // std::cout << "------------------------------------------" << std::endl;

        // Convert to unix epoch seconds format, for example,
        //  + 1745644603 --> 2025-04-26 07:16:43
        //  + 1745817403 --> 2025-04-28 07:16:43
        // Note: recent times seem not working, only 2025-04, 05, 06, 07 works???
        // std::string start_timestamp = "2025-06-26 07:16:10";
        // std::string end_timestamp   = "2025-06-26 07:26:10";
        uint64_t sta_ts = timestamp2unix_epoch_seconds(start_timestamp);
        uint64_t end_ts = timestamp2unix_epoch_seconds(end_timestamp);
        std::cout << " + Start Timestamp: " << start_timestamp << " --> " << sta_ts << std::endl;
        std::cout << " + End Timestamp:   " << end_timestamp   << " --> " << end_ts << std::endl;
        std::cout << "------------------------------------------" << std::endl;

        // Query the sensor data
        std::vector<int64_t> sensor0_data = mqss_dcdb_query_sensor_data(session, query, sensor_dev, sta_ts, end_ts);
        std::cout << "------------------------------------------" << std::endl;
        std::cout << "Sensor Data Values (" << sensor0_data.size() << " samples): ";
        for (auto value : sensor0_data){
            std::cout << value << " ";
        }
        std::cout << std::endl;
        std::cout << "------------------------------------------" << std::endl;

        // Tear down the session and clean up
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