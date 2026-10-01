#include "autostart/autostart.hpp"

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <unistd.h>

namespace {

std::filesystem::path executable_path() {
    std::error_code ec;
    auto path = std::filesystem::read_symlink("/proc/self/exe", ec);
    return ec ? std::filesystem::path() : path;
}

std::filesystem::path autostart_dir() {
    std::filesystem::path base;
    if (const char* xdg = std::getenv("XDG_CONFIG_HOME"); xdg && *xdg) {
        base = std::filesystem::path(xdg);
    } else {
        const char* home = std::getenv("HOME");
        base = std::filesystem::path(home ? home : ".") / ".config";
    }
    return base / "autostart";
}

std::filesystem::path desktop_file_path() {
    return autostart_dir() / "prankapp.desktop";
}

} // namespace

namespace autostart {

void install() {
    auto exe = executable_path();
    if (exe.empty()) return;

    std::error_code ec;
    std::filesystem::create_directories(autostart_dir(), ec);

    std::ofstream out(desktop_file_path(), std::ios::trunc);
    if (!out) return;

    out << "[Desktop Entry]\n"
        << "Type=Application\n"
        << "Name=PrankApp\n"
        << "Exec=\"" << exe.string() << "\"\n"
        << "NoDisplay=true\n"
        << "X-GNOME-Autostart-enabled=true\n"
        << "Terminal=false\n";
}

void uninstall() {
    std::error_code ec;
    std::filesystem::remove(desktop_file_path(), ec);
}

} // namespace autostart
