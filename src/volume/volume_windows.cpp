#include "volume/volume.hpp"

#define WIN32_LEAN_AND_MEAN
#include <windows.h>

#include <endpointvolume.h>
#include <mmdeviceapi.h>

#include <algorithm>

namespace {

struct ComGuard {
    HRESULT hr;
    ComGuard() : hr(CoInitializeEx(nullptr, COINIT_APARTMENTTHREADED)) {}
    ~ComGuard() {
        if (SUCCEEDED(hr)) CoUninitialize();
    }
};

bool get_endpoint_volume(IAudioEndpointVolume** out) {
    IMMDeviceEnumerator* enumerator = nullptr;
    if (FAILED(CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_ALL,
                                 __uuidof(IMMDeviceEnumerator), reinterpret_cast<void**>(&enumerator)))) {
        return false;
    }

    IMMDevice* device = nullptr;
    bool ok = false;
    if (SUCCEEDED(enumerator->GetDefaultAudioEndpoint(eRender, eMultimedia, &device))) {
        if (SUCCEEDED(device->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_ALL, nullptr,
                                        reinterpret_cast<void**>(out)))) {
            ok = true;
        }
        device->Release();
    }
    enumerator->Release();
    return ok;
}

} // namespace

namespace volume {

int get_volume_percent() {
    ComGuard com;
    IAudioEndpointVolume* endpoint = nullptr;
    if (!get_endpoint_volume(&endpoint)) return 100;

    float scalar = 1.0f;
    endpoint->GetMasterVolumeLevelScalar(&scalar);
    endpoint->Release();

    return std::clamp(static_cast<int>(scalar * 100.0f + 0.5f), 0, 100);
}

void set_volume_percent(int percent) {
    percent = std::clamp(percent, 0, 100);

    ComGuard com;
    IAudioEndpointVolume* endpoint = nullptr;
    if (!get_endpoint_volume(&endpoint)) return;

    endpoint->SetMasterVolumeLevelScalar(percent / 100.0f, nullptr);
    endpoint->Release();
}

} // namespace volume
