#include "volume/volume.hpp"

#include <algorithm>
#include <array>
#include <cstdio>
#include <cstdlib>
#include <memory>
#include <regex>
#include <string>

namespace {

bool command_exists(const char* cmd) {
    std::string check = std::string("command -v ") + cmd + " >/dev/null 2>&1";
    return std::system(check.c_str()) == 0;
}

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

int parse_first_percent(const std::string& text, int fallback) {
    static const std::regex pattern(R"((\d{1,3})%)");
    std::smatch match;
    if (std::regex_search(text, match, pattern)) {
        return std::clamp(std::stoi(match[1].str()), 0, 100);
    }
    return fallback;
}

} // namespace

namespace volume {

int get_volume_percent() {
    if (command_exists("pactl")) {
        return parse_first_percent(run_command_capture("pactl get-sink-volume @DEFAULT_SINK@ 2>/dev/null"), 100);
    }
    if (command_exists("amixer")) {
        return parse_first_percent(run_command_capture("amixer get Master 2>/dev/null"), 100);
    }
    return 100;
}

void set_volume_percent(int percent) {
    percent = std::clamp(percent, 0, 100);
    std::string pct = std::to_string(percent) + "%";

    if (command_exists("pactl")) {
        std::system(("pactl set-sink-volume @DEFAULT_SINK@ " + pct + " >/dev/null 2>&1").c_str());
        return;
    }
    if (command_exists("amixer")) {
        if (std::system(("amixer -D pulse sset Master " + pct + " >/dev/null 2>&1").c_str()) == 0) return;
        std::system(("amixer sset Master " + pct + " >/dev/null 2>&1").c_str());
    }
}

} // namespace volume
