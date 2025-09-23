#include "daemon.hpp"
#include <iostream>
#include <cstdlib>

int main(int argc, char **argv)
{

    Config cfg;
    cfg.daemon_id = 101;
    cfg.daemon_name = "mqss-dcdb-daemon";
    cfg.dcdb_host_server = "host.docker.internal:9042";

    try
    {
        Daemon dm(cfg);
        std::cout << "----------------------------------------" << std::endl;
        std::cout << "MQSS QDMI-based DCDB Daemon.            " << std::endl;
        std::cout << "----------------------------------------" << std::endl;

        // Create a QDMI session for QDMI-based DCDB Device access
        static DCDB_QDMI_Device_Session session;

        // Allocate and intialize the session
        DCDB_QDMI_device_initialize();
        DCDB_QDMI_device_session_alloc(&session);
        if (session == nullptr)
        {
            std::cerr << "Fatal: Error when allocating QDMI Device session for DCDB" << std::endl;
            return 1;
        }
        DCDB_QDMI_device_session_set_parameter(session, QDMI_DEVICE_SESSION_PARAMETER_BASEURL,
                                               cfg.dcdb_host_server.size(),
                                               cfg.dcdb_host_server.c_str());
        DCDB_QDMI_device_session_init(session);

        std::cout << "QDMI-based DCDB Device is initialized successfully." << std::endl;
        std::cout << "MQSS QDMI-based DCDB Daemon initialized successfully." << std::endl;
        std::cout << "MQSS DCDB Daemon ID: " << cfg.daemon_id << std::endl;
        std::cout << "MQSS DCDB Daemon Name: " << cfg.daemon_name << std::endl;
        std::cout << "DCDB Host Server: " << cfg.dcdb_host_server << std::endl;

        // Try to get something
        std::cout << "------------------------------------------" << std::endl;
        size_t size = 0;
        DCDB_QDMI_device_session_query_device_property(session, QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, 0, nullptr, &size);

        std::vector<DCDB_QDMI_TelemetrySensor> envs(size / sizeof(DCDB_QDMI_TelemetrySensor));
        DCDB_QDMI_device_session_query_device_property(session, QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, size, static_cast<void *>(envs.data()), nullptr);

        DCDB_QDMI_TelemetrySensor env = envs.at(0);
        DCDB_QDMI_Device_TelemetrySensor_Query query;
        DCDB_QDMI_device_session_create_device_telemetrysensor_query(session, &query);

        // Timestamp to get the sensor data
        uint64_t sta_ts = 1745644603;
        uint64_t end_ts = 1745817403;
        size_t timestamp_size;
        size_t result_size;

        QDMI_TelemetrySensor_Query_Status status;

        // set the start timestamp
        DCDB_QDMI_device_telemetrysensor_query_set_parameter(query, QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_STARTTIME,
                                                             sizeof(uint64_t), &sta_ts);

        // set the end timestamp
        DCDB_QDMI_device_telemetrysensor_query_set_parameter(query, QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_ENDTIME,
                                                             sizeof(uint64_t), &end_ts);

        // set the query parameters
        DCDB_QDMI_device_telemetrysensor_query_set_parameter(query, QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_TELEMETRYSENSOR,
                                                             sizeof(DCDB_QDMI_TelemetrySensor), &env);

        // check the sensor id name
        size_t id_size;
        DCDB_QDMI_device_session_query_telemetrysensor_property(session, env, QDMI_TELEMETRYSENSOR_PROPERTY_ID, 0, nullptr, &id_size);
        std::string id(id_size - 1, '\0');
        DCDB_QDMI_device_session_query_telemetrysensor_property(session, env, QDMI_TELEMETRYSENSOR_PROPERTY_ID, id_size, static_cast<void *>(id.data()), nullptr);

        // submit the query
        DCDB_QDMI_device_telemetrysensor_query_submit(query);

        // wait for the query
        DCDB_QDMI_device_telemetrysensor_query_wait(query, 0);

        // check the query status
        DCDB_QDMI_device_telemetrysensor_query_check_status(query, &status);

        // get the query result size
        DCDB_QDMI_device_telemetrysensor_query_get_results(query, QDMI_TELEMETRYSENSOR_QUERY_RESULT_TIMESTAMPS,
                                                           0, nullptr, &timestamp_size);

        // get the timestamp details
        std::vector<uint64_t> timestamps(timestamp_size / sizeof(uint64_t));
        DCDB_QDMI_device_telemetrysensor_query_get_results(query, QDMI_TELEMETRYSENSOR_QUERY_RESULT_TIMESTAMPS,
                                                           timestamp_size, static_cast<void *>(timestamps.data()), nullptr);

        // get the query result size
        DCDB_QDMI_device_telemetrysensor_query_get_results(query, QDMI_TELEMETRYSENSOR_QUERY_RESULT_VALUES,
                                                           0, nullptr, &result_size);

        std::vector<int64_t> values(result_size / sizeof(int64_t));
        DCDB_QDMI_device_telemetrysensor_query_get_results(query, QDMI_TELEMETRYSENSOR_QUERY_RESULT_VALUES,
                                                           result_size, static_cast<void *>(values.data()), nullptr);

        std::cout << "------------------------------------------" << std::endl;
        std::cout << "QDMI-based DCDB Query: " << id << std::endl;
        std::cout << "------------------------------------------" << std::endl;
        for (auto value : values){
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
    catch (const std::exception &ex)
    {
        std::cerr << "Fatal: " << ex.what() << std::endl;
        return 1;
    }
}