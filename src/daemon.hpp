
#include <cstdint>
#include <string>
#include <vector>


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
