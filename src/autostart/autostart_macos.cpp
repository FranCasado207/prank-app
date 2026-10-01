#include "autostart/autostart.hpp"

#include <climits>
#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <mach-o/dyld.h>

namespace {

std::filesystem::path executable_path() {
    std::vector<char> buf(PATH_MAX);
    uint32_t size = static_cast<uint32_t>(buf.size());
    if (_NSGetExecutablePath(buf.data(), &size) != 0) {
        buf.resize(size);
        if (_NSGetExecutablePath(buf.data(), &size) != 0) return {};
    }
    std::error_code ec;
    auto resolved = std::filesystem::canonical(buf.data(), ec);
    return ec ? std::filesystem::path(buf.data()) : resolved;
}

std::filesystem::path launch_agents_dir() {
    const char* home = std::getenv("HOME");
    return std::filesystem::path(home ? home : ".") / "Library" / "LaunchAgents";
}

std::filesystem::path plist_path() {
    return launch_agents_dir() / "com.prankapp.agent.plist";
}

} // namespace

namespace autostart {

void install() {
    auto exe = executable_path();
    if (exe.empty()) return;

    std::error_code ec;
    std::filesystem::create_directories(launch_agents_dir(), ec);

    std::ofstream out(plist_path(), std::ios::trunc);
    if (!out) return;

    out << "<?xml version=\"1.0\" encoding=\"UTF-8\"?>\n"
        << "<!DOCTYPE plist PUBLIC \"-//Apple//DTD PLIST 1.0//EN\" \"http://www.apple.com/DTDs/PropertyList-1.0.dtd\">\n"
        << "<plist version=\"1.0\"><dict>\n"
        << "  <key>Label</key><string>com.prankapp.agent</string>\n"
        << "  <key>ProgramArguments</key><array><string>" << exe.string() << "</string></array>\n"
        << "  <key>RunAtLoad</key><true/>\n"
        << "  <key>KeepAlive</key><false/>\n"
        << "</dict></plist>\n";
}

void uninstall() {
    auto path = plist_path();
    std::string cmd = "launchctl unload -w \"" + path.string() + "\" >/dev/null 2>&1";
    std::system(cmd.c_str());

    std::error_code ec;
    std::filesystem::remove(path, ec);
}

} // namespace autostart
