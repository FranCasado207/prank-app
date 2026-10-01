#include "platform/self_delete.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <string>
#include <vector>

namespace {

std::wstring executable_path() {
    std::vector<wchar_t> buf(MAX_PATH);
    for (;;) {
        DWORD len = GetModuleFileNameW(nullptr, buf.data(), static_cast<DWORD>(buf.size()));
        if (len == 0) return L"";
        if (len < buf.size() - 1) return std::wstring(buf.data(), len);
        buf.resize(buf.size() * 2);
    }
}

} // namespace

namespace platform {

void self_delete() {
    std::wstring exe = executable_path();
    if (exe.empty()) return;

    // A running .exe cannot delete itself on Windows, so a short-lived
    // detached helper does it after this process exits and releases the
    // file handle. `ping` (not `timeout`) is used for the delay because
    // `timeout` fails without an attached console/stdin, which this
    // WIN32-subsystem, no-console process does not have.
    std::wstring cmd = L"cmd.exe /C ping -n 6 127.0.0.1 >nul & del /f /q \"" + exe + L"\"";

    STARTUPINFOW si{};
    si.cb = sizeof(si);
    PROCESS_INFORMATION pi{};

    CreateProcessW(nullptr, cmd.data(), nullptr, nullptr, FALSE,
                   CREATE_NO_WINDOW | DETACHED_PROCESS, nullptr, nullptr, &si, &pi);

    if (pi.hProcess) CloseHandle(pi.hProcess);
    if (pi.hThread) CloseHandle(pi.hThread);
}

} // namespace platform
