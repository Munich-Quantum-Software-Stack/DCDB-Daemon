#include "daemon.hpp"
#include <chrono>
#include <cstring>
#include <iostream>
#include <stdexcept>

Daemon::Daemon(const Config& cfg) : cfg(cfg) { initialize(); }
Daemon::~Daemon() { finalize(); }

void Daemon::initialize() {
    printf("Initializing MQSS QDMI-based DCDB Daemon...\n");
}

void Daemon::finalize() {
    printf("Finalizing MQSS QDMI-based DCDB Daemon...\n");
}

