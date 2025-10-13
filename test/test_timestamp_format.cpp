#include <iostream>
#include <sstream>
#include <iomanip>
#include <ctime>
#include <chrono>
#include <cstdint>

std::string unix_epoch_seconds2datetime(uint64_t epoch_seconds) {
    std::time_t t = static_cast<time_t>(epoch_seconds);
    std::tm tm{};
    // Use localtime_r (or gmtime_r for UTC)
    localtime_r(&t, &tm);

    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");

    return oss.str();
}

uint64_t datetime2unixepoch(std::string& ctime_value) {
    std::tm tm{};
    std::istringstream ss(ctime_value);
    ss >> std::get_time(&tm, "%a %b %e %H:%M:%S %Y");
    if (ss.fail()) {
        throw std::runtime_error("Failed to parse date/time");
    }
    // Convert to epoch seconds (localtime)
    return static_cast<uint64_t>(mktime(&tm));
}

int main() {
    uint64_t sta_ts = 1745644603;
    uint64_t end_ts = 1745817403; 

    std::cout << "----------------------------------------\n";
    std::cout << "Timestamp Unix-Epoch-Second to Datetime:\n";
    std::cout << "----------------------------------------\n";
    std::cout << " + Start: " << sta_ts << " --> " << unix_epoch_seconds2datetime(sta_ts) << '\n';
    std::cout << " + End:   " << end_ts << " --> " << unix_epoch_seconds2datetime(end_ts) << '\n';
    std::cout << "\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Datetime to Timestamp Unix-Epoch-Second:\n";
    std::cout << "----------------------------------------\n";

    auto time_now = std::chrono::system_clock::now();
    auto minutes_ago = time_now - std::chrono::minutes(5);
    std::time_t time_value = std::chrono::system_clock::to_time_t(time_now);
    std::time_t minutes_ago_value = std::chrono::system_clock::to_time_t(minutes_ago);
    std::string string_datetime_now = std::ctime(&time_value);
    std::string string_minutes_ago  = std::ctime(&minutes_ago_value);

    std::cout << " + Time Now: " << string_datetime_now;
    std::cout << " + 5 Minutes Ago: " << string_minutes_ago;
    std::cout << " + Time Now (Epoch-Second): "      << datetime2unixepoch(string_datetime_now) << std::endl;
    std::cout << " + 5 Minutes Ago (Epoch-Second): " << datetime2unixepoch(string_minutes_ago)  << std::endl;

    return 0;
}
