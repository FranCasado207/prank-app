#include "autostart/autostart.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <shlobj.h>
#include <shobjidl.h>

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

std::wstring shortcut_path() {
    PWSTR startup = nullptr;
    std::wstring result;
    if (SUCCEEDED(SHGetKnownFolderPath(FOLDERID_Startup, 0, nullptr, &startup))) {
        result = std::wstring(startup) + L"\\PrankApp.lnk";
        CoTaskMemFree(startup);
    }
    return result;
}

} // namespace

namespace autostart {

void install() {
    std::wstring exe = executable_path();
    std::wstring link_path = shortcut_path();
    if (exe.empty() || link_path.empty()) return;

    HRESULT init_hr = CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED);
    bool need_uninit = SUCCEEDED(init_hr);

    IShellLinkW* link = nullptr;
    if (SUCCEEDED(CoCreateInstance(CLSID_ShellLink, nullptr, CLSCTX_INPROC_SERVER,
                                    IID_IShellLinkW, reinterpret_cast<void**>(&link)))) {
        link->SetPath(exe.c_str());

        auto slash = exe.find_last_of(L"\\/");
        if (slash != std::wstring::npos) {
            link->SetWorkingDirectory(exe.substr(0, slash).c_str());
        }

        IPersistFile* file = nullptr;
        if (SUCCEEDED(link->QueryInterface(IID_IPersistFile, reinterpret_cast<void**>(&file)))) {
            file->Save(link_path.c_str(), TRUE);
            file->Release();
        }
        link->Release();
    }

    if (need_uninit) CoUninitialize();
}

void uninstall() {
    std::wstring link_path = shortcut_path();
    if (!link_path.empty()) DeleteFileW(link_path.c_str());
}

} // namespace autostart
