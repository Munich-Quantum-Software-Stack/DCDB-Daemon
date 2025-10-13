#include "daemon.hpp"
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>

Daemon::Daemon(const Config& cfg) : cfg(cfg) { initialize(); }
Daemon::~Daemon() { finalize(); }

/**
 * @brief Initialize the MQSS QDMI-based DCDB Daemon.
 *
 * This function performs the setup required before the daemon 
 * can start interacting with the QDMI-based DCDB device library.
 *
 * @note This current function prototype just prints an informational message 
 * to the console when initialization begins.
 */
void Daemon::initialize() {
    std::cout << "------------------------------------------" << std::endl;
    printf("Initializing MQSS QDMI-based DCDB Daemon...\n");
    std::cout << "------------------------------------------" << std::endl;
}

/**
 * @brief Finalize the MQSS QDMI-based DCDB Daemon.
 *
 * This function performs finalizing the daemon and stop
 * interacting with the QDMI-based DCDB device library.
 *
 * @note This current function prototype just prints an informational message 
 * to the console when initialization begins.
 */
void Daemon::finalize() {
    std::cout << "------------------------------------------" << std::endl;
    std::cout << "Finalizing MQSS QDMI-based DCDB Daemon...\n";
    std::cout << "------------------------------------------" << std::endl;
}

/**
 * @brief Query telemetry sensor data from a QDMI-based DCDB device.
 *
 * This function performs a telemetry query for a specific sensor device
 * within a DCDB QDMI device session over a given time window. It configures
 * the telemetry query with the start and end timestamps, submits the query
 * to the QDMI device, waits for retrieving the resulting sensor values.
 *
 * The function returns a vector of integer values representing the sampled
 * telemetry data points within the specified time range.
 *
 * @param session          Active QDMI device session handle.
 * @param query            A telemetry sensor query handle created for this session.
 * @param sensor_device    The telemetry sensor to query.
 * @param start_timestamp  Start of the query time window (UNIX epoch seconds).
 * @param end_timestamp    End of the query time window (UNIX epoch seconds).
 *
 * @return std::vector<int64_t>  
 *         A vector containing the queried sensor data values. The size of the
 *         vector corresponds to the number of samples retrieved.
 *
 * @note This function blocks until the query completes or is canceled.
 *
 * @see DCDB_QDMI_device_session_create_telemetrysensor_query()
 * @see DCDB_QDMI_device_telemetrysensor_query_submit()
 * @see DCDB_QDMI_device_telemetrysensor_query_get_results()
 */
std::vector<int64_t> mqss_dcdb_query_sensor_data(DCDB_QDMI_Device_Session session, 
    DCDB_QDMI_Device_TelemetrySensor_Query query, 
    DCDB_QDMI_TelemetrySensor sensor_device, 
    uint64_t start_timestamp, 
    uint64_t end_timestamp)
{
    QDMI_TelemetrySensor_Query_Status status;
    size_t timestamp_size;
    size_t result_size;

    // set the start timestamp
    DCDB_QDMI_device_telemetrysensor_query_set_parameter(query, QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_STARTTIME,
                                                        sizeof(uint64_t), &start_timestamp);

    // set the end timestamp
    DCDB_QDMI_device_telemetrysensor_query_set_parameter(query, QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_ENDTIME,
                                                        sizeof(uint64_t), &end_timestamp);

    // set the query parameters
    DCDB_QDMI_device_telemetrysensor_query_set_parameter(query, QDMI_DEVICE_TELEMETRYSENSOR_QUERY_PARAMETER_TELEMETRYSENSOR,
                                                        sizeof(DCDB_QDMI_TelemetrySensor), 
                                                        &sensor_device);

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

    std::vector<int64_t> values(result_size/sizeof(int64_t));
    DCDB_QDMI_device_telemetrysensor_query_get_results(query, QDMI_TELEMETRYSENSOR_QUERY_RESULT_VALUES,
                                                    result_size, static_cast<void *>(values.data()), nullptr);

    return values;
}

/**
 * @brief Check the sensor id from a given list of available DCDB sensors.
 *
 * This function performs checking the name of the sensor based on the input device
 * ID for a specific sensor device. The function returns a string of sensor device path
 * or name defined in advance.
 *
 * @param session          Active QDMI device session handle.
 * @param sensor_device    A pointer to a sensor device ID to be checked.
 *
 * @return std::string
 *         A string value as the name/path of the sensor device within DCDB.
 */
std::string check_sensor_device_id(DCDB_QDMI_Device_Session session, 
    DCDB_QDMI_TelemetrySensor sensor_device)
{
    size_t sensor_name_size;
    DCDB_QDMI_device_session_query_telemetrysensor_property(session, sensor_device, QDMI_TELEMETRYSENSOR_PROPERTY_ID, 
                                                            0, nullptr, &sensor_name_size);
    std::string sensor_name(sensor_name_size-1, NULL);
    DCDB_QDMI_device_session_query_telemetrysensor_property(session, sensor_device, QDMI_TELEMETRYSENSOR_PROPERTY_ID, 
                                                            sensor_name_size, static_cast<void *>(sensor_name.data()), nullptr);
    return sensor_name;
}

/**
 * @brief Convert the unix-epoch-second format to the datetime string format.
 *
 * This function performs convertin the timestamp format in unix-epoch-seconds
 * to the normal format that users can read. For example, 2025-10-09 12:30:45.
 *
 * @param epoch_seconds    Timestamp value in unix-epoch-seconds format.
 *
 * @return std::string
 *         A string value as the datetime format, i.e., YYYY-MM-DD HH:MM:SS.
 */
std::string unix_epoch_seconds2datetime(uint64_t epoch_seconds) {
    std::time_t t = static_cast<time_t>(epoch_seconds);
    std::tm tm{};
    // Use localtime_r (or gmtime_r for UTC)
    localtime_r(&t, &tm);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    return oss.str();
}


/**
 * @brief Convert the ctime format to unix-epoch-second format for querying.
 *
 * This function performs converting the ctime format into unix-epoch-seconds
 * that MQSS DCDB Daemon can check and query the sensor data in a specific timeframe.
 *
 * @param ctime_value    The datetime value in ctime format.
 *
 * @return uint64_t
 *         A value as the unix-epoch-seconds format, e.g., 
 *         Thu Oct 9 12:32:18 2025 to 1760009538
 */
uint64_t datetime2unix_epoch_seconds(std::string& ctime_value) {
    std::tm tm{};
    std::istringstream ss(ctime_value);
    ss >> std::get_time(&tm, "%a %b %e %H:%M:%S %Y");
    if (ss.fail()) {
        throw std::runtime_error("Failed to parse date/time. The datetime should be: Day Mon DD HH:MM:SS YYYY.");
    }
    // Convert to epoch seconds (localtime)
    return static_cast<uint64_t>(mktime(&tm));
}

/**
 * @brief Convert the timestamp format to unix-epoch-second format for querying.
 *
 * This function performs converting the timestamp format into unix-epoch-seconds
 * that MQSS DCDB Daemon can check and query the sensor data in a specific timeframe.
 * For example, 2025-10-09 12:30:45 to 1760009445.
 *
 * @param timestamp_string    The datetime value in ctime format.
 *
 * @return uint64_t
 *         A value as the unix-epoch-seconds format, e.g., 
 *         2025-10-09 12:30:45 to 1760009445
 */
uint64_t timestamp2unix_epoch_seconds(std::string& timestamp_string)
{
    std::tm tm{};
    std::istringstream ss(timestamp_string);
    ss >> std::get_time(&tm, "%Y-%m-%d %H:%M:%S");
    if (ss.fail()) {
        throw std::runtime_error("Failed to parse date/time. The timestamp should be: YYYY-MM-DD HH:MM:SS.");
    }
    // Convert to epoch seconds (localtime)
    return static_cast<uint64_t>(mktime(&tm));
}