#include "scheduler.hpp"

#include "audio.hpp"
#include "instance/single_instance.hpp"
#include "volume/volume.hpp"

#include <chrono>
#include <random>
#include <thread>

namespace scheduler {

void run_loop(const SchedulerConfig& config) {
    std::mt19937 rng(std::random_device{}());
    std::bernoulli_distribution roll(config.probability);

    constexpr auto poll_interval = std::chrono::seconds(2);
    const auto cycle = std::chrono::duration_cast<std::chrono::seconds>(config.cycle);

    for (;;) {
        for (auto elapsed = std::chrono::seconds(0); elapsed < cycle; elapsed += poll_interval) {
            std::this_thread::sleep_for(poll_interval);
            if (instance::stop_requested()) return;
        }

        if (roll(rng)) {
            int saved_volume = volume::get_volume_percent();
            volume::set_volume_percent(50);
            audio::play_random_embedded_sound();
            volume::set_volume_percent(saved_volume);
        }
    }
}

} // namespace scheduler
