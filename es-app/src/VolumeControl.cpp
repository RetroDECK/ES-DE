//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  VolumeControl.cpp
//
//  Controls system audio volume.
//

#include "VolumeControl.h"

#include "Log.h"
#include "utils/MathUtil.h"

#if defined(_WIN64)
#include <cmath>
#endif

#if defined(__linux__)
std::string VolumeControl::mixerName = "Master";
std::string VolumeControl::mixerCard = "default";
#endif

VolumeControl::VolumeControl()
// clang-format off
#if defined(__linux__)
    : mixerIndex {0}
    , mixerHandle {nullptr}
    , mixerElem {nullptr}
    , mixerSelemId {nullptr}
#elif defined(_WIN64)
    : mixerHandle {nullptr}
    , endpointVolume {nullptr}
#endif
// clang-format on
{
    init();
}

VolumeControl::~VolumeControl()
{
    deinit();
#if defined(__linux__)
    snd_config_update_free_global();
#endif
}

void VolumeControl::init()
{
    // Initialize audio mixer interface.

#if defined(__linux__)
    // Try to open mixer device.
    if (mixerHandle == nullptr) {
        snd_mixer_selem_id_alloca(&mixerSelemId);
        // Sets simple-mixer index and name.
        snd_mixer_selem_id_set_index(mixerSelemId, mixerIndex);
        snd_mixer_selem_id_set_name(mixerSelemId, mixerName.c_str());
        if (snd_mixer_open(&mixerHandle, 0) >= 0) {
            LOG(LogDebug) << "VolumeControl::init(): Opened ALSA mixer";
            if (snd_mixer_attach(mixerHandle, mixerCard.c_str()) >= 0) {
                LOG(LogDebug) << "VolumeControl::init(): Attached to default card";
                if (snd_mixer_selem_register(mixerHandle, nullptr, nullptr) >= 0) {
                    LOG(LogDebug) << "VolumeControl::init(): Registered simple element class";
                    if (snd_mixer_load(mixerHandle) >= 0) {
                        LOG(LogDebug) << "VolumeControl::init(): Loaded mixer elements";
                        // Find elements.
                        mixerElem = snd_mixer_find_selem(mixerHandle, mixerSelemId);
                        if (mixerElem != nullptr) {
                            LOG(LogDebug) << "VolumeControl::init(): Mixer initialized";
                        }
                        else {
                            LOG(LogError)
                                << "VolumeControl::init(): Failed to find mixer elements!";
                            snd_mixer_close(mixerHandle);
                            mixerHandle = nullptr;
                        }
                    }
                    else {
                        LOG(LogError) << "VolumeControl::init(): Failed to load mixer elements!";
                        snd_mixer_close(mixerHandle);
                        mixerHandle = nullptr;
                    }
                }
                else {
                    LOG(LogError)
                        << "VolumeControl::init(): Failed to register simple element class!";
                    snd_mixer_close(mixerHandle);
                    mixerHandle = nullptr;
                }
            }
            else {
                LOG(LogError) << "VolumeControl::init(): Failed to attach to default card!";
                snd_mixer_close(mixerHandle);
                mixerHandle = nullptr;
            }
        }
        else {
            LOG(LogError) << "VolumeControl::init(): Failed to open ALSA mixer!";
        }
    }
#elif defined(_WIN64)
    // Windows Vista or above.
    if (endpointVolume == nullptr) {
        CoInitialize(nullptr);
        IMMDeviceEnumerator* deviceEnumerator = nullptr;
        CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER,
                         __uuidof(IMMDeviceEnumerator),
                         reinterpret_cast<LPVOID*>(&deviceEnumerator));
        if (deviceEnumerator != nullptr) {
            // Get default endpoint.
            IMMDevice* defaultDevice = nullptr;
            deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
            if (defaultDevice != nullptr) {
                // Retrieve endpoint volume.
                defaultDevice->Activate(__uuidof(IAudioEndpointVolume), CLSCTX_INPROC_SERVER,
                                        nullptr, reinterpret_cast<LPVOID*>(&endpointVolume));
                if (endpointVolume == nullptr) {
                    LOG(LogError) << "VolumeControl::init(): "
                                     "Failed to get default audio endpoint volume!";
                }
                // Release default device. we don't need it anymore.
                defaultDevice->Release();
            }
            else {
                LOG(LogError) << "VolumeControl::init(): Failed to get default audio endpoint!";
            }
            // Release device enumerator. we don't need it anymore.
            deviceEnumerator->Release();
        }
        else {
            LOG(LogError) << "VolumeControl::init(): Failed to get audio endpoint enumerator!";
            CoUninitialize();
        }
    }
#endif
}

void VolumeControl::deinit()
{
    // Deinitialize audio mixer interface.

#if defined(__linux__)
    if (mixerHandle != nullptr) {
        snd_mixer_detach(mixerHandle, mixerCard.c_str());
        snd_mixer_free(mixerHandle);
        snd_mixer_close(mixerHandle);
        mixerHandle = nullptr;
        mixerElem = nullptr;
    }
#elif defined(_WIN64)
    if (endpointVolume != nullptr) {
        endpointVolume->Release();
        endpointVolume = nullptr;
        CoUninitialize();
    }
#endif
}

int VolumeControl::getVolume() const
{
    int volume = 0;

#if defined(__linux__)
    if (mixerElem != nullptr) {
        // Get volume range.
        long minVolume;
        long maxVolume;
        if (snd_mixer_selem_get_playback_volume_range(mixerElem, &minVolume, &maxVolume) == 0) {
            long rawVolume;
            if (snd_mixer_selem_get_playback_volume(mixerElem, SND_MIXER_SCHN_MONO, &rawVolume) ==
                0) {
                // Bring into range 0-100.
                rawVolume -= minVolume;
                if (rawVolume > 0)
                    volume = (rawVolume * 100.0) / (maxVolume - minVolume) + 0.5;
            }
            else {
                LOG(LogError) << "VolumeControl::getVolume(): Failed to get mixer volume";
            }
        }
        else {
            LOG(LogError) << "VolumeControl::getVolume(): Failed to get volume range";
        }
    }
#elif defined(_WIN64)
    if (endpointVolume != nullptr) {
        // Windows Vista or above, uses EndpointVolume API.
        float floatVolume = 0.0f; // 0-1
        if (endpointVolume->GetMasterVolumeLevelScalar(&floatVolume) == S_OK) {
            volume = static_cast<int>(std::round(floatVolume * 100.0f));
            LOG(LogInfo) << "System audio volume is " << volume;
        }
        else {
            LOG(LogError) << "VolumeControl::getVolume(): Failed to get master volume!";
        }
    }
#endif

    volume = glm::clamp(volume, 0, 100);
    return volume;
}

void VolumeControl::setVolume(int volume)
{
    volume = glm::clamp(volume, 0, 100);

#if defined(__linux__)
    if (mixerElem != nullptr) {
        // Get volume range.
        long minVolume;
        long maxVolume;
        if (snd_mixer_selem_get_playback_volume_range(mixerElem, &minVolume, &maxVolume) == 0) {
            // Bring into minVolume-maxVolume range and set.
            long rawVolume = (volume * (maxVolume - minVolume) / 100) + minVolume;
            if (snd_mixer_selem_set_playback_volume(mixerElem, SND_MIXER_SCHN_FRONT_LEFT,
                                                    rawVolume) < 0 ||
                snd_mixer_selem_set_playback_volume(mixerElem, SND_MIXER_SCHN_FRONT_RIGHT,
                                                    rawVolume) < 0) {
                LOG(LogError) << "VolumeControl::getVolume(): Failed to set mixer volume";
            }
        }
        else {
            LOG(LogError) << "VolumeControl::getVolume(): Failed to get volume range";
        }
    }
#elif defined(_WIN64)
    if (endpointVolume != nullptr) {
        // Windows Vista or above, uses EndpointVolume API.
        float floatVolume = 0.0f; // 0-1
        if (volume > 0)
            floatVolume = static_cast<float>(volume) / 100.0f;
        if (endpointVolume->SetMasterVolumeLevelScalar(floatVolume, nullptr) != S_OK) {
            LOG(LogError) << "VolumeControl::setVolume(): Failed to set master volume";
        }
    }
#endif
}
