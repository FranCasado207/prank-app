#pragma once

#include <chrono>

namespace scheduler {

struct SchedulerConfig {
    std::chrono::minutes cycle{30};
    double probability{0.04};
};

// Runs until instance::stop_requested() becomes true (checked every ~2s).
void run_loop(const SchedulerConfig& config);

} // namespace scheduler
