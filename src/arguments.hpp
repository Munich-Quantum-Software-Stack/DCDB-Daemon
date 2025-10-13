
#pragma once
#include <string>
#include <optional>
#include <stdexcept>
#include <iostream>
#include <unordered_map>

struct Arguments {
    std::string dcdb_url = "host.docker.internal:9042";
    std::string sensor_id;                    // required
    std::optional<std::string> start_time;    // "YYYY-MM-DD HH:MM:SS"
    std::optional<std::string> end_time;      // "YYYY-MM-DD HH:MM:SS"
    std::optional<int> minutes_ago;           // alternative to start+end
    bool verbose = false;
};

inline void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog 
              << " --sensor-id <ID> "
              << " [--minutes-ago N] " 
              << " [--start \"YYYY-MM-DD HH:MM:SS\"] "
              << " [--end \"YYYY-MM-DD HH:MM:SS\"] "
              << " [--dcdb-url HOST:PORT] "
              << " [--verbose]\n"
              << "\n"
              << "For examples:\n"
              << "  " << prog << " --sensor-id 101 --minutes-ago 10\n"
              << "  " << prog << " --sensor-id 101 --start \"2025-06-25 07:16:10\" --end \"2025-06-25 07:26:10\"\n";
}

inline bool is_flag(const std::string& s) { 
    return !s.empty() && s.rfind("-",0) == 0; 
}

inline Arguments parse_args(int argc, char** argv) {

    // The struct object to hold parsed arguments
    Arguments arguments;

    // Check the number of input arguments
    if (argc == 1) {
        print_usage(argv[0]); 
        throw std::runtime_error("Missing arguments");
    } 
    
    // Parse the input arguments
    for (int i = 1; i < argc; ++i) {
    
        std::string key = argv[i];
        
        if (key == "-h" || key == "--help") {
            print_usage(argv[0]);
            std::exit(0);
        }
        else if (key == "-v" || key == "--verbose") { 
            arguments.verbose = true; 
        }
        else if (key == "--sensor-id") {
            if (i+1 >= argc || is_flag(argv[i+1])) 
                throw std::runtime_error("--sensor-id needs a value");
            arguments.sensor_id = argv[++i];
        }
        else if (key == "--dcdb-url") {
            if (i+1 >= argc || is_flag(argv[i+1])) 
                throw std::runtime_error("--dcdb-url needs a value");
            arguments.dcdb_url = argv[++i];
        }
        else if (key == "--minutes-ago") {
            if (i+1 >= argc || is_flag(argv[i+1])) 
                throw std::runtime_error("--minutes-ago needs N");

            arguments.minutes_ago = std::stoi(argv[++i]);
            
            if (*arguments.minutes_ago <= 0) 
                throw std::runtime_error("--minutes-ago must be > 0");
        }
        else if (key == "--start") {
            if (i+1 >= argc || is_flag(argv[i+1])) 
                throw std::runtime_error("--start needs a datetime");
            arguments.start_time = argv[++i];
        }
        else if (key == "--end") {
            if (i+1 >= argc || is_flag(argv[i+1])) 
                throw std::runtime_error("--end needs a datetime");
            arguments.end_time = argv[++i];
        }
        else {
            throw std::runtime_error("unknown option: " + key);
        }
    }

    if (arguments.sensor_id.empty()) 
        throw std::runtime_error("--sensor-id is required");

    const bool have_range = arguments.start_time.has_value() || arguments.end_time.has_value();
    const bool have_rel   = arguments.minutes_ago.has_value();

    if (have_rel && have_range) 
        throw std::runtime_error("use either --minutes-ago or --start/--end, not both");
    if (have_range && (!arguments.start_time || !arguments.end_time)) 
        throw std::runtime_error("--start and --end must be provided together");
    if (!have_rel && !have_range) 
        throw std::runtime_error("provide either --minutes-ago N or --start/--end");

    return arguments;
}
