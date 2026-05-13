#include "daemon.hpp"
#include "arguments.hpp"
#include "database.hpp"
#include "config.hpp"

#include <iostream>
#include <fstream>
#include <sstream>
#include <cstdlib>
#include <chrono>
#include <atomic>
#include <csignal>
#include <thread>
#include <cerrno>
#include <cstring>
#include <sys/wait.h>

// TeeStreambuf: duplicates all writes to both a primary and a secondary streambuf.
// Used so that std::cout / std::cerr write to terminal AND log file simultaneously.
struct TeeStreambuf : std::streambuf {
    TeeStreambuf(std::streambuf* primary, std::streambuf* secondary)
        : primary_(primary), secondary_(secondary) {}

protected:
    int overflow(int c) override {
        if (c == EOF) return EOF;
        if (primary_->sputc(c) == EOF) return EOF;
        secondary_->sputc(c);  // best-effort; ignore file errors
        return c;
    }
    std::streamsize xsputn(const char* s, std::streamsize n) override {
        std::streamsize written = primary_->sputn(s, n);
        secondary_->sputn(s, n);
        return written;
    }
    int sync() override {
        int r = primary_->pubsync();
        secondary_->pubsync();  // flush file buffer to disk
        return r;
    }

private:
    std::streambuf* primary_;
    std::streambuf* secondary_;
};

// State file helpers
static uint64_t read_state_file(const std::string& path) {
    std::ifstream f(path);
    if (!f.is_open()) return 0;
    uint64_t ts = 0;
    f >> ts;
    return ts;
}

static void write_state_file(const std::string& path, uint64_t end_ts) {
    std::ofstream f(path, std::ios::trunc);
    if (f.is_open()) f << end_ts << '\n';
}

// Global flag — set to false when Ctrl+C is pressed
static std::atomic<bool> running{true};

// Returns current wall-clock time as "HH:MM:SS" for log prefixes
static std::string ts() {
    std::time_t t = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
    char buf[9];
    struct tm tm_buf;
    std::strftime(buf, sizeof(buf), "%H:%M:%S", localtime_r(&t, &tm_buf));
    return buf;
}

// Signal handler — called on Ctrl+C (SIGINT) or kill (SIGTERM)
static void signal_handler(int signal) {
    std::cout << '\n' << ts() << " signal " << signal << " — stopping\n";
    running = false;
}

static void crash_handler(int signal) {
    std::cerr << "\nCRASH signal " << signal << " — stopping\n";
    running = false;
}

int main(int argc, char **argv)
{
    // Register signal handlers
    std::signal(SIGINT,  signal_handler);   // Ctrl+C
    std::signal(SIGTERM, signal_handler);   // kill command

    // std::signal(SIGSEGV, crash_handler);  // segfault
    // std::signal(SIGABRT, crash_handler);  // abort (heap corruption)

    // Step 1a: Parse and validate arguments first
    Arguments args = parse_args(argc, argv);

    // Step 1b: Set up log file tee (stdout + file)
    std::ofstream log_stream;
    std::unique_ptr<TeeStreambuf> tee_out, tee_err;
    std::streambuf* orig_cout = nullptr;
    std::streambuf* orig_cerr = nullptr;
    if (!args.log_file.empty()) {
        log_stream.open(args.log_file, std::ios::app);
        if (!log_stream.is_open()) {
            std::cerr << ts() << " WARN could not open log file: " << args.log_file << '\n';
        } else {
            orig_cout = std::cout.rdbuf();
            orig_cerr = std::cerr.rdbuf();
            tee_out = std::make_unique<TeeStreambuf>(orig_cout, log_stream.rdbuf());
            tee_err = std::make_unique<TeeStreambuf>(orig_cerr, log_stream.rdbuf());
            std::cout.rdbuf(tee_out.get());
            std::cerr.rdbuf(tee_err.get());
        }
    }

    std::cout << ts() << " starting (Ctrl+C to stop)\n";
    if (!args.log_file.empty())
        std::cout << ts() << " log → " << args.log_file << '\n';

    // Step 1c: Load JSON config
    AppConfig app_cfg;
    try {
        app_cfg = load_config(args.config_path);
    } catch (const std::exception& e) {
        std::cerr << ts() << " FATAL " << e.what() << '\n';
        return 1;
    }
    std::cout << ts() << " config " << args.config_path << " (" << app_cfg.data_sources.size() << " sensors)\n";

    // Step 2: Connect to InfluxDB
    auto db = influxdb::InfluxDBFactory::Get(app_cfg.influx_url);
    if (!db) {
        std::cerr << ts() << " FATAL could not connect to InfluxDB\n";
        return 1;
    }
    std::cout << ts() << " InfluxDB " << app_cfg.influx_url << '\n';

    // Step 3: Initialize DCDB Config
    Config cfg;
    cfg.daemon_id        = 101;
    cfg.daemon_name      = "mqss-dcdb-daemon";
    // Use --dcdb-url if provided, otherwise fall back to the default (host.docker.internal:9042).
    cfg.dcdb_host_server = args.dcdb_url;
    // NOTE: dcdb_host_server is the address of the DCDB/Cassandra (ScyllaDB) backend.
    // - On Mac with Docker Desktop: "host.docker.internal:9042" reaches the host machine
    //   from inside a container (Docker Desktop provides this DNS name automatically).
    // - On a Linux VM running the daemon directly (no container): use "localhost:9042"
    //   or the VM's IP address, e.g. "192.168.1.10:9042".
    // - On a Linux VM with the daemon inside a container: use the VM's host IP or the
    //   Docker bridge gateway (typically "172.17.0.1:9042"), NOT "host.docker.internal"
    //   (that DNS name is only available on Docker Desktop for Mac/Windows).
    // This value can also be overridden at runtime via the --dcdb-url flag.

    try {
        Daemon dm(cfg);

        size_t size = 0;
        static DCDB_QDMI_Device_Session session;

        // Allocate and intialize the QDMI session
        DCDB_QDMI_device_initialize();
        DCDB_QDMI_device_session_alloc(&session);
        if (session == nullptr) {
            std::cerr << ts() << " FATAL QDMI session alloc failed\n";
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

        std::cout << ts() << " DCDB ready (ID=" << cfg.daemon_id << ", host=" << cfg.dcdb_host_server << ")\n";

        // Step 4a: Read last state (only when --resume is passed)
        uint64_t last_end_ts = 0;
        if (args.resume) {
            last_end_ts = read_state_file(args.state_file);
            if (last_end_ts > 0) {
                std::time_t t_last = static_cast<std::time_t>(last_end_ts);
                std::string str_last = std::ctime(&t_last);
                if (!str_last.empty() && str_last.back() == '\n') str_last.pop_back();
                std::cout << ts() << " resume " << args.state_file << ": last_end=" << str_last << '\n';
            } else {
                std::cout << ts() << " resume: no state file — starting from now-" << args.minutes_ago.value_or(5) << "min\n";
            }
        }

        // Step 4: Main loop
        while (running) {
            auto loop_start = std::chrono::steady_clock::now();

            // Query window: [sta_ts, end_ts]
            // On the first iteration after a restart, resume from last_end_ts so no
            // data is missed. On subsequent iterations use [now-5min, now].
            const int duration_minutes = args.minutes_ago.value_or(5);
            auto time_now = std::chrono::system_clock::now();
            uint64_t end_ts = static_cast<uint64_t>(std::chrono::system_clock::to_time_t(time_now));
            uint64_t sta_ts;
            // Catch-up: query from where we left off to now
            if (last_end_ts > 0) {
                sta_ts = last_end_ts;
            } else {
                auto time_start = time_now - std::chrono::minutes(duration_minutes);
                sta_ts = static_cast<uint64_t>(std::chrono::system_clock::to_time_t(time_start));
            }
            // Clear so subsequent loops use the rolling window
            last_end_ts = 0;

            std::time_t t_start = static_cast<std::time_t>(sta_ts);
            std::time_t t_end   = static_cast<std::time_t>(end_ts);
            std::string str_start = std::ctime(&t_start);
            std::string str_end   = std::ctime(&t_end);
            if (!str_start.empty() && str_start.back() == '\n') str_start.pop_back();
            if (!str_end.empty()   && str_end.back()   == '\n') str_end.pop_back();

            std::cout << ts() << " window [" << str_start << " → " << str_end << "]\n";

            // --- Query and write ---
            // Each sensor runs in a child process so that a library crash
            // (heap corruption → SIGABRT) only kills the child, not the daemon.
            for (const auto& src : app_cfg.data_sources) {

                if (!running) break;

                std::cout << ts() << " sensor[" << src.sensor_id << "] "
                          << src.measurement << '/' << src.field_name
                          << " (÷" << src.divisor << ")\n";

                pid_t child_pid = fork();
                if (child_pid < 0) {
                    std::cerr << ts() << " WARN sensor[" << src.sensor_id << "] fork failed: " << std::strerror(errno) << "\n";
                    continue;
                }

                if (child_pid == 0) {
                    // ---- Child process ----
                    // Restore original streambufs so the child is no longer writing
                    // through the parent's TeeStreambuf/filebuf (not fork-safe).
                    if (orig_cout) std::cout.rdbuf(orig_cout);
                    if (orig_cerr) std::cerr.rdbuf(orig_cerr);

                    // Open a fresh log stream for this child (append mode).
                    // _exit() skips C++ destructors, so we flush explicitly before
                    // every exit point below.
                    std::ofstream child_log;
                    std::unique_ptr<TeeStreambuf> child_tee_out, child_tee_err;
                    if (!args.log_file.empty()) {
                        child_log.open(args.log_file, std::ios::app);
                        if (child_log.is_open()) {
                            child_tee_out = std::make_unique<TeeStreambuf>(
                                std::cout.rdbuf(), child_log.rdbuf());
                            child_tee_err = std::make_unique<TeeStreambuf>(
                                std::cerr.rdbuf(), child_log.rdbuf());
                            std::cout.rdbuf(child_tee_out.get());
                            std::cerr.rdbuf(child_tee_err.get());
                        }
                    }

                    // Ignore Ctrl+C and SIGTERM — the parent handles shutdown and will
                    // SIGKILL us if needed. Without this, both parent and child would
                    // print the signal message and the child would never exit cleanly.
                    std::signal(SIGINT,  SIG_IGN);
                    std::signal(SIGTERM, SIG_IGN);

                    // Create a fresh QDMI session — the parent's session is not
                    // fork-safe (internal threads/state are not replicated into the
                    // child), so query_wait would block forever on the parent's session.
                    try {
                        DCDB_QDMI_Device_Session child_session;
                        DCDB_QDMI_device_initialize();
                        DCDB_QDMI_device_session_alloc(&child_session);
                        DCDB_QDMI_device_session_set_parameter(child_session,
                            QDMI_DEVICE_SESSION_PARAMETER_BASEURL,
                            cfg.dcdb_host_server.size(),
                            cfg.dcdb_host_server.c_str());
                        DCDB_QDMI_device_session_init(child_session);

                        // Re-enumerate sensors on the fresh session
                        size_t child_size = 0;
                        DCDB_QDMI_device_session_query_device_property(child_session,
                            QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, 0, nullptr, &child_size);
                        std::vector<DCDB_QDMI_TelemetrySensor> child_envs(
                            child_size / sizeof(DCDB_QDMI_TelemetrySensor));
                        DCDB_QDMI_device_session_query_device_property(child_session,
                            QDMI_DEVICE_PROPERTY_TELEMETRYSENSORS, child_size,
                            static_cast<void*>(child_envs.data()), nullptr);

                        DCDB_QDMI_Device_TelemetrySensor_Query child_query;
                        auto child_db = influxdb::InfluxDBFactory::Get(app_cfg.influx_url);

                        DCDB_QDMI_device_session_create_device_telemetrysensor_query(child_session, &child_query);
                        DCDB_QDMI_TelemetrySensor sensor_dev = child_envs.at(src.sensor_id);
                        std::string sensor_name = check_sensor_device_id(child_session, sensor_dev);

                        DCDBSensorQueryResult query_results = mqss_dcdb_query_sensor_data(
                            child_session, child_query, sensor_dev, sta_ts, end_ts);

                        write_to_influxdb(*child_db, query_results, sensor_name, src);

                        DCDB_QDMI_device_telemetrysensor_query_free(child_query);
                        DCDB_QDMI_device_session_free(child_session);
                        DCDB_QDMI_device_finalize();
                    } catch (const std::exception& e) {
                        std::cerr << ts() << " sensor[" << src.sensor_id << "] exception: " << e.what() << '\n';
                        std::cout.flush(); std::cerr.flush();
                        _exit(1);
                    }
                    std::cout.flush(); std::cerr.flush();
                    _exit(0);
                }

                // ---- Parent process: wait for child with timeout ----
                int status = 0;
                const int timeout_seconds = 10;
                bool timed_out = true;

                for (int i = 0; i < timeout_seconds; ++i) {
                    pid_t ret = waitpid(child_pid, &status, WNOHANG);
                    if (ret == child_pid) { timed_out = false; break; }
                    if (!running) break;  // Ctrl+C — let fall through to kill
                    std::this_thread::sleep_for(std::chrono::seconds(1));
                }

                if (timed_out || !running) {
                    kill(child_pid, SIGKILL);
                    waitpid(child_pid, &status, 0);
                    std::cerr << ts() << " WARN sensor[" << src.sensor_id << "] timed out (" << timeout_seconds << "s) — killed\n";
                } else if (WIFSIGNALED(status)) {
                    std::cerr << ts() << " WARN sensor[" << src.sensor_id << "] crashed (sig " << WTERMSIG(status) << ") — skipping\n";
                } else if (WIFEXITED(status) && WEXITSTATUS(status) != 0) {
                    std::cerr << ts() << " WARN sensor[" << src.sensor_id << "] failed — skipping\n";
                }
            }

            // --- Persist state so a restart can resume from here ---
            if (!args.state_file.empty()) {
                write_state_file(args.state_file, end_ts);
                std::cout << ts() << " state saved: " << end_ts << " → " << args.state_file << '\n';
            }

            // --- Sleep for duration_minutes, but wake up early if stopped ---
            auto next_run = loop_start + std::chrono::minutes(duration_minutes);
            while (running && std::chrono::steady_clock::now() < next_run) {
                std::this_thread::sleep_for(std::chrono::seconds(1));
            }
        }

        // Tear down the session and clean up
        std::cout << ts() << " shutting down\n";
        DCDB_QDMI_device_session_free(session);
        DCDB_QDMI_device_finalize();

        // Restore original cout/cerr streambufs before log_stream is destroyed
        if (orig_cout) std::cout.rdbuf(orig_cout);
        if (orig_cerr) std::cerr.rdbuf(orig_cerr);

        return 0;
    }
    catch (const std::exception &ex){
        std::cerr << ts() << " FATAL " << ex.what() << '\n';
        // Restore original streambufs on error path too
        if (orig_cout) std::cout.rdbuf(orig_cout);
        if (orig_cerr) std::cerr.rdbuf(orig_cerr);
        return 1;
    }
}