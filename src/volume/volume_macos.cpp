#include "volume/volume.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <string>

namespace {

std::string run_command_capture(const std::string& cmd) {
    std::array<char, 256> buffer{};
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd.c_str(), "r"), pclose);
    if (!pipe) return result;
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

} // namespace

namespace volume {

int get_volume_percent() {
    auto out = run_command_capture("osascript -e 'output volume of (get volume settings)' 2>/dev/null");
    try {
        return std::clamp(std::stoi(out), 0, 100);
    } catch (...) {
        return 100;
    }
}

void set_volume_percent(int percent) {
    percent = std::clamp(percent, 0, 100);
    std::string cmd = "osascript -e 'set volume output volume " + std::to_string(percent) + "' >/dev/null 2>&1";
    std::system(cmd.c_str());
}

} // namespace volume
