
#pragma once
#include <string>
#include <optional>
#include <stdexcept>
#include <iostream>
#include <unordered_map>

struct Arguments {
    std::string dcdb_url    = "host.docker.internal:9042";
    std::string config_path = "config.json";        // path to JSON config file
    std::string log_file    = "";                   // empty = stdout/stderr only
    std::string state_file  = "daemon_state.txt";   // persists last queried end_ts
    std::optional<int> minutes_ago;                 // query window: N minutes ago from now
    bool resume  = false;   // if true, resume querying from last state file timestamp
    bool verbose = false;
};

inline void print_usage(const char* prog) {
    std::cerr << "Usage: " << prog
              << " [--config PATH]"
              << " [--minutes-ago N]"
              << " [--dcdb-url HOST:PORT]"
              << " [--log-file PATH]"
              << " [--state-file PATH]"
              << " [--resume]"
              << " [--verbose]\n"
              << "\n"
              << "Sensors are read from the config file (default: config.json).\n"
              << "If --minutes-ago is omitted, the daemon uses a 5-minute rolling window.\n"
              << "If --log-file is set, all output is written to the file in addition to stdout/stderr.\n"
              << "--resume: on startup, read the state file and catch up from the last queried\n"
              << "  timestamp instead of starting from [now - window]. The state file is always\n"
              << "  written after each loop so it is available for future --resume runs.\n"
              << "\n"
              << "Examples:\n"
              << "  " << prog << "\n"
              << "  " << prog << " --minutes-ago 10\n"
              << "  " << prog << " --config /etc/dcdb/config.json --resume\n"
              << "  " << prog << " --config /etc/dcdb/config.json --log-file /var/log/daemon.log\n";
}

inline bool is_flag(const std::string& s) { 
    return !s.empty() && s.rfind("-",0) == 0; 
}

inline Arguments parse_args(int argc, char** argv) {

    // The struct object to hold parsed arguments
    Arguments arguments;

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
        else if (key == "--resume") {
            arguments.resume = true;
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
        else if (key == "--log-file") {
            if (i+1 >= argc || is_flag(argv[i+1]))
                throw std::runtime_error("--log-file needs a path");
            arguments.log_file = argv[++i];
        }
        else if (key == "--state-file") {
            if (i+1 >= argc || is_flag(argv[i+1]))
                throw std::runtime_error("--state-file needs a path");
            arguments.state_file = argv[++i];
        }
        else {
            throw std::runtime_error("unknown option: " + key);
        }
    }

    return arguments;
}
