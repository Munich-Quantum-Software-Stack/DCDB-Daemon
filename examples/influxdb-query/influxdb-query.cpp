#include <InfluxDB/InfluxDBFactory.h>
#include <InfluxDB/Point.h>
#include <iostream>
#include <chrono>
#include <thread>

int main() {
    try {
        auto db = influxdb::InfluxDBFactory::Get(
            "http://account:credentials"
            "@portal-telemetry.de:8086?db=database_name");

        // Check databases
        auto response1 = db->execute("SHOW DATABASES");
        std::cout << "Databases: " << response1 << "\n";

        auto response2 = db->execute("SELECT * FROM environmental LIMIT 5");
        std::cout << "Data: " << response2 << "\n";

        // --- WRITE ---
        std::cout << "Writing test point...\n";
        db->write(influxdb::Point{"test_measurement"}
            .addTag("source", "debug_test")
            .addField("value", 99.9));
        std::cout << "Write done.\n";

        // --- READ BACK ---
        std::this_thread::sleep_for(std::chrono::seconds(1)); // give InfluxDB a moment
        auto response = db->execute("SELECT * FROM test_measurement ORDER BY time DESC LIMIT 5");
        std::cout << "Read back: " << response << "\n";

    } catch (const std::exception& e) {
        std::cerr << "ERROR: " << e.what() << '\n';
        return 1;
    }
    return 0;
}