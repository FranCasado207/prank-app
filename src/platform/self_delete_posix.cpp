#include "platform/self_delete.hpp"

#include <filesystem>

#if defined(__APPLE__)
#include <climits>
#include <vector>

#include <mach-o/dyld.h>
#else
#endif

namespace {

std::filesystem::path executable_path() {
#if defined(__APPLE__)
    std::vector<char> buf(PATH_MAX);
    uint32_t size = static_cast<uint32_t>(buf.size());
    if (_NSGetExecutablePath(buf.data(), &size) != 0) {
        buf.resize(size);
        if (_NSGetExecutablePath(buf.data(), &size) != 0) return {};
    }
    std::error_code ec;
    auto resolved = std::filesystem::canonical(buf.data(), ec);
    return ec ? std::filesystem::path(buf.data()) : resolved;
#else
    std::error_code ec;
    auto path = std::filesystem::read_symlink("/proc/self/exe", ec);
    return ec ? std::filesystem::path() : path;
#endif
}

} // namespace

namespace platform {

void self_delete() {
    auto exe = executable_path();
    if (exe.empty()) return;
    std::error_code ec;
    std::filesystem::remove(exe, ec);
}

} // namespace platform
