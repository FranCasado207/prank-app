#include "instance/single_instance.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <shlobj.h>

namespace instance {

std::filesystem::path app_data_dir() {
    PWSTR path = nullptr;
    std::filesystem::path base;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &path))) {
        base = std::filesystem::path(path) / "PrankApp";
        CoTaskMemFree(path);
    } else {
        base = std::filesystem::path(".") / "PrankApp";
    }
    std::error_code ec;
    std::filesystem::create_directories(base, ec);
    return base;
}

bool try_acquire_lock(LockHandle& out) {
    HANDLE mutex = CreateMutexW(nullptr, TRUE, L"Local\\PrankApp_SingleInstance");
    if (!mutex) return false;
    if (GetLastError() == ERROR_ALREADY_EXISTS) {
        CloseHandle(mutex);
        return false;
    }
    out.mutex_handle = mutex;
    return true;
}

void release_lock(LockHandle& handle) {
    if (handle.mutex_handle) {
        HANDLE mutex = static_cast<HANDLE>(handle.mutex_handle);
        ReleaseMutex(mutex);
        CloseHandle(mutex);
        handle.mutex_handle = nullptr;
    }
}

} // namespace instance
