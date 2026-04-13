#pragma once

#include "database.hpp"

#include <nlohmann/json.hpp>
#include <fstream>
#include <stdexcept>
#include <string>
#include <vector>

// ---------------------------------------------------------------
// AppConfig: runtime configuration loaded from config.json
// ---------------------------------------------------------------
struct AppConfig {
    std::string             influx_url;
    std::vector<DataSource> data_sources;  // only enabled entries
};

// ---------------------------------------------------------------
// load_config: parse config.json and return an AppConfig.
// Throws std::runtime_error on any I/O or parse failure.
// Only DataSource entries with "enabled": true are included.
// ---------------------------------------------------------------
inline AppConfig load_config(const std::string& path)
{
    std::ifstream file(path);
    if (!file.is_open())
        throw std::runtime_error("Cannot open config file: " + path);

    nlohmann::json j;
    try {
        file >> j;
    } catch (const nlohmann::json::parse_error& e) {
        throw std::runtime_error("JSON parse error in " + path + ": " + e.what());
    }

    AppConfig cfg;

    if (!j.contains("influx_url") || !j["influx_url"].is_string())
        throw std::runtime_error("config.json: missing or invalid 'influx_url'");
    cfg.influx_url = j["influx_url"].get<std::string>();

    if (!j.contains("data_sources") || !j["data_sources"].is_array())
        throw std::runtime_error("config.json: missing or invalid 'data_sources'");

    for (const auto& entry : j["data_sources"]) {
        // Skip disabled sensors
        if (entry.contains("enabled") && !entry["enabled"].get<bool>())
            continue;

        DataSource src;
        src.sensor_id   = entry.at("sensor_id").get<int>();
        src.divisor     = entry.at("divisor").get<double>();
        src.measurement = entry.at("measurement").get<std::string>();
        src.field_name  = entry.at("field_name").get<std::string>();
        cfg.data_sources.push_back(std::move(src));
    }

    return cfg;
}
