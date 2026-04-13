
#pragma once
#include <string>
#include <optional>
#include <stdexcept>
#include <iostream>
#include <unordered_map>

struct Arguments {
    std::string dcdb_url    = "host.docker.internal:9042";
    std::string config_path = "config.json";  // path to JSON config file
    std::optional<int> minutes_ago;           // query window: N minutes ago from now
    bool verbose = false;
};

inline void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog
              << " [--config PATH] "
              << " [--minutes-ago N] "
              << " [--dcdb-url HOST:PORT] "
              << " [--verbose]\n"
              << "\n"
              << "Sensors are read from the config file (default: config.json).\n"
              << "If --minutes-ago is omitted, the daemon uses its built-in rolling window.\n"
              << "\n"
              << "Examples:\n"
              << "  " << prog << "\n"
              << "  " << prog << " --minutes-ago 10\n"
              << "  " << prog << " --config /etc/dcdb/config.json --minutes-ago 10\n";
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
        else if (key == "--config") {
            if (i+1 >= argc || is_flag(argv[i+1]))
                throw std::runtime_error("--config needs a path");
            arguments.config_path = argv[++i];
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
        else {
            throw std::runtime_error("unknown option: " + key);
        }
    }

    return arguments;
}
