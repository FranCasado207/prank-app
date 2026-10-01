#include "instance/single_instance.hpp"

#include <cstdlib>
#include <fcntl.h>
#include <sys/file.h>
#include <unistd.h>

namespace instance {

std::filesystem::path app_data_dir() {
    std::filesystem::path base;
#if defined(__APPLE__)
    const char* home = std::getenv("HOME");
    base = std::filesystem::path(home ? home : ".") / "Library" / "Application Support" / "PrankApp";
#else
    if (const char* xdg = std::getenv("XDG_DATA_HOME"); xdg && *xdg) {
        base = std::filesystem::path(xdg) / "PrankApp";
    } else {
        const char* home = std::getenv("HOME");
        base = std::filesystem::path(home ? home : ".") / ".local" / "share" / "PrankApp";
    }
#endif
    std::error_code ec;
    std::filesystem::create_directories(base, ec);
    return base;
}

bool try_acquire_lock(LockHandle& out) {
    auto lock_path = app_data_dir() / "prankapp.lock";
    int fd = ::open(lock_path.c_str(), O_CREAT | O_RDWR, 0600);
    if (fd < 0) return false;
    if (::flock(fd, LOCK_EX | LOCK_NB) != 0) {
        ::close(fd);
        return false;
    }
    out.fd = fd;
    return true;
}

void release_lock(LockHandle& handle) {
    if (handle.fd >= 0) {
        ::flock(handle.fd, LOCK_UN);
        ::close(handle.fd);
        handle.fd = -1;
    }
}

} // namespace instance
