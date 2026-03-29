
#include <cstdint>
#include <string>
#include <vector>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>

#include <dcdb_qdmi/device.h>
#include <dcdb_qdmi/types.h>
#include <sensors.h>
#include <session.h>

struct Config {
    int daemon_id;
    std::string daemon_name;
    std::string dcdb_host_server; // e.g. host.docker.internal:9042
};

class Daemon {
public:
    explicit Daemon(const Config& cfg);
    ~Daemon();

    // Runs a handful of calls to ensure the stack works end-to-end.
    // Returns 0 on success, non-zero QDMI error otherwise.

private:
    // Lifecycle
    void initialize();
    void finalize();

    const Config cfg;
};

struct DCDBSensorQueryResult {
    // UNIX epoch nanoseconds from DCDB
    std::vector<uint64_t> timestamps;
    std::vector<int64_t>  values;
};

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
DCDBSensorQueryResult mqss_dcdb_query_sensor_data(DCDB_QDMI_Device_Session session, 
    DCDB_QDMI_Device_TelemetrySensor_Query query, 
    DCDB_QDMI_TelemetrySensor sensor_device, 
    uint64_t start_timestamp, 
    uint64_t end_timestamp);

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
    DCDB_QDMI_TelemetrySensor sensor_device);

/* ----------------------------------------------------------
 * Utility functions for the daemon
 * ----------------------------------------------------------
 */

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
std::string unix_epoch_seconds2datetime(uint64_t epoch_seconds);

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
uint64_t datetime2unix_epoch_seconds(std::string& ctime_value);

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
uint64_t timestamp2unix_epoch_seconds(std::string& timestamp_string);