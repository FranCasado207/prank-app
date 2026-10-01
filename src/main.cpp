#include "autostart/autostart.hpp"
#include "instance/single_instance.hpp"
#include "platform/self_delete.hpp"
#include "scheduler.hpp"

#include <chrono>
#include <cstring>
#include <iostream>
#include <thread>

#if !defined(_WIN32)
#include <fcntl.h>
#include <unistd.h>
#endif

namespace {

struct CliArgs {
    bool uninstall = false;
    bool foreground = false;
    bool test_mode = false;
};

CliArgs parse_args(int argc, char** argv) {
    CliArgs args;
    for (int i = 1; i < argc; ++i) {
        if (std::strcmp(argv[i], "--uninstall") == 0) {
            args.uninstall = true;
        } else if (std::strcmp(argv[i], "--foreground") == 0) {
            args.foreground = true;
        } else if (std::strcmp(argv[i], "--test-mode") == 0) {
            args.test_mode = true;
        }
    }
    return args;
}

int run_uninstall() {
    instance::request_stop();

    // Best-effort wait for a running instance to release its lock, just to
    // give the caller accurate feedback; not required for correctness below.
    instance::LockHandle probe;
    for (int i = 0; i < 20; ++i) {
        if (instance::try_acquire_lock(probe)) {
            instance::release_lock(probe);
            break;
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(500));
    }

    autostart::uninstall();
    instance::clear_stop_signal();
    platform::self_delete();

    std::cout << "PrankApp uninstalled." << std::endl;
    return 0;
}

#if !defined(_WIN32)
void daemonize() {
    pid_t pid = fork();
    if (pid > 0) std::exit(0); // parent detaches, child continues in background
    if (pid < 0) return;       // fork failed: fall back to running in foreground

    setsid();

    int devnull = open("/dev/null", O_RDWR);
    if (devnull >= 0) {
        dup2(devnull, STDIN_FILENO);
        dup2(devnull, STDOUT_FILENO);
        dup2(devnull, STDERR_FILENO);
        if (devnull > STDERR_FILENO) close(devnull);
    }
}
#endif

} // namespace

int main(int argc, char** argv) {
    CliArgs args = parse_args(argc, argv);

    if (args.uninstall) return run_uninstall();

    instance::LockHandle lock;
    if (!instance::try_acquire_lock(lock)) return 0; // another instance is already running
    instance::clear_stop_signal();

    if (args.foreground) {
        std::cout << "PrankApp running in foreground"
                  << (args.test_mode ? " (test mode: 100% chance every 1 min)" : "") << std::endl;
    } else {
        autostart::install();
#if !defined(_WIN32)
        daemonize();
#endif
    }

    scheduler::SchedulerConfig config;
    if (args.test_mode) {
        config.cycle = std::chrono::minutes(1);
        config.probability = 1.0;
    }

    scheduler::run_loop(config);
    instance::release_lock(lock);
    return 0;
}
