#pragma once

#include <filesystem>
#include <fstream>
#include <system_error>

namespace instance {

struct LockHandle {
#if defined(_WIN32)
    void* mutex_handle = nullptr;
#else
    int fd = -1;
#endif
};

// Implemented per-platform in single_instance_windows.cpp / single_instance_posix.cpp.
std::filesystem::path app_data_dir();
bool try_acquire_lock(LockHandle& out);
void release_lock(LockHandle& handle);

inline std::filesystem::path stop_signal_path() {
    return app_data_dir() / "prankapp.stop";
}

inline void request_stop() {
    std::ofstream(stop_signal_path(), std::ios::trunc).put('1');
}

inline bool stop_requested() {
    std::error_code ec;
    return std::filesystem::exists(stop_signal_path(), ec);
}

inline void clear_stop_signal() {
    std::error_code ec;
    std::filesystem::remove(stop_signal_path(), ec);
}

} // namespace instance
