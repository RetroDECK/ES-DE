//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  VolumeControl.cpp
//
//  Controls system audio volume.
//

#include "VolumeControl.h"

#include "math/Misc.h"
#include "Log.h"

#if defined(_RPI_)
#include "Settings.h"
#endif

// The ALSA Audio Card and Audio Device selection code is disabled at the moment.
// As PulseAudio controls the sound devices for the desktop environment, it doesn't
// make much sense to be able to select ALSA devices directly. Normally (always?)
// the selection doesn't make any difference at all. But maybe some PulseAudio
// settings could be added later on, if needed.
// The code is still active for Raspberry Pi though as I'm not sure if this is
// useful for that device.
// Keeping mixerName and mixerCard at their default values should make sure that
// the rest of the volume control code in here compiles and works fine.
#if defined(__linux__)
#if defined(_RPI_) || defined(_VERO4K_)
std::string VolumeControl::mixerName = "PCM";
#else
std::string VolumeControl::mixerName = "Master";
#endif
std::string VolumeControl::mixerCard = "default";
#endif

std::weak_ptr<VolumeControl> VolumeControl::sInstance;

VolumeControl::VolumeControl()
        : originalVolume(0),
        internalVolume(0)
        #if defined(__APPLE__)
//        #error TODO: Not implemented for MacOS yet!!!
        #elif defined(__linux__)
        , mixerIndex(0),
        mixerHandle(nullptr),
        mixerElem(nullptr),
        mixerSelemId(nullptr)
        #elif defined(_WIN64)
        , mixerHandle(nullptr),
        endpointVolume(nullptr)
        #endif
{
    init();

    // Get original volume levels for system.
    originalVolume = getVolume();
}

VolumeControl::VolumeControl(
        const VolumeControl& right):
        originalVolume(0),
        internalVolume(0)
        #if defined(__APPLE__)
//        #error TODO: Not implemented for MacOS yet!!!
        #elif defined(__linux__)
        , mixerIndex(0),
        mixerHandle(nullptr),
        mixerElem(nullptr),
        mixerSelemId(nullptr)
        #elif defined(_WIN64)
        , mixerHandle(nullptr),
        endpointVolume(nullptr)
        #endif
{
    static_cast<void>(right);
    sInstance = right.sInstance;
}

VolumeControl & VolumeControl::operator=(const VolumeControl& right)
{
    if (this != &right)
        sInstance = right.sInstance;

    return *this;
}

VolumeControl::~VolumeControl()
{
    // Set original volume levels for system.
    //setVolume(originalVolume);

    deinit();
}

std::shared_ptr<VolumeControl>& VolumeControl::getInstance()
{
    // Check if an VolumeControl instance is already created, if not create one.
    static std::shared_ptr<VolumeControl> sharedInstance = sInstance.lock();
    if (sharedInstance == nullptr) {
        sharedInstance.reset(new VolumeControl);
        sInstance = sharedInstance;
    }
    return sharedInstance;
}

void VolumeControl::init()
{
    // Initialize audio mixer interface.
    #if defined(__APPLE__)
//    #error TODO: Not implemented for MacOS yet!!!
    #elif defined(__linux__)
    // Try to open mixer device.
    if (mixerHandle == nullptr) {
        // Allow user to override the AudioCard and AudioDevice in es_settings.cfg.
        #if defined(_RPI_)
        mixerCard = Settings::getInstance()->getString("AudioCard");
        mixerName = Settings::getInstance()->getString("AudioDevice");
        #endif

        snd_mixer_selem_id_alloca(&mixerSelemId);
        // Sets simple-mixer index and name.
        snd_mixer_selem_id_set_index(mixerSelemId, mixerIndex);
        snd_mixer_selem_id_set_name(mixerSelemId, mixerName.c_str());
        // Open mixer.
        if (snd_mixer_open(&mixerHandle, 0) >= 0) {
            LOG(LogDebug) << "VolumeControl::init() - Opened ALSA mixer";
            // Ok, attach to defualt card.
            if (snd_mixer_attach(mixerHandle, mixerCard.c_str()) >= 0) {
                LOG(LogDebug) << "VolumeControl::init() - Attached to default card";
                // Ok, register simple element class.
                if (snd_mixer_selem_register(mixerHandle, nullptr, nullptr) >= 0) {
                    LOG(LogDebug) << "VolumeControl::init() - Registered simple element class";
                    // Ok, load registered elements.
                    if (snd_mixer_load(mixerHandle) >= 0) {
                        LOG(LogDebug) << "VolumeControl::init() - Loaded mixer elements";
                        // Ok, find elements now.
                        mixerElem = snd_mixer_find_selem(mixerHandle, mixerSelemId);
                        if (mixerElem != nullptr) {
                            // Wohoo. good to go...
                            LOG(LogDebug) << "VolumeControl::init() - Mixer initialized";
                        }
                        else {
                            LOG(LogError) <<
                                    "VolumeControl::init() - Failed to find mixer elements!";
                            snd_mixer_close(mixerHandle);
                            mixerHandle = nullptr;
                        }
                    }
                    else {
                        LOG(LogError) << "VolumeControl::init() - Failed to load mixer elements!";
                        snd_mixer_close(mixerHandle);
                        mixerHandle = nullptr;
                    }
                }
                else {
                    LOG(LogError) <<
                            "VolumeControl::init() - Failed to register simple element class!";
                    snd_mixer_close(mixerHandle);
                    mixerHandle = nullptr;
                }
            }
            else {
                LOG(LogError) << "VolumeControl::init() - Failed to attach to default card!";
                snd_mixer_close(mixerHandle);
                mixerHandle = nullptr;
            }
        }
        else {
            LOG(LogError) << "VolumeControl::init() - Failed to open ALSA mixer!";
        }
    }
    #elif defined(_WIN64)
    // Get windows version information.
    OSVERSIONINFOEXA osVer = { sizeof(OSVERSIONINFO) };
    ::GetVersionExA(reinterpret_cast<LPOSVERSIONINFOA>(&osVer));
    // Check windows version.
    if (osVer.dwMajorVersion < 6) {
        // Windows older than Vista. use mixer API. open default mixer.
        if (mixerHandle == nullptr) {
            #if defined(_WIN64)
            if (mixerOpen(&mixerHandle, 0, reinterpret_cast<DWORD_PTR>(nullptr), 0, 0) ==
                    MMSYSERR_NOERROR) {
            #else
            if (mixerOpen(&mixerHandle, 0, nullptr, 0, 0) == MMSYSERR_NOERROR) {
            #endif
                // Retrieve info on the volume slider control for the "Speaker Out" line.
                MIXERLINECONTROLS mixerLineControls;
                mixerLineControls.cbStruct = sizeof(MIXERLINECONTROLS);
                mixerLineControls.dwLineID = 0xFFFF0000; // Id of "Speaker Out" line.
                mixerLineControls.cControls = 1;
                // Id of "Speaker Out" line's volume slider.
                //mixerLineControls.dwControlID = 0x00000000;
                // Get volume control.
                mixerLineControls.dwControlType = MIXERCONTROL_CONTROLTYPE_VOLUME;
                mixerLineControls.pamxctrl = &mixerControl;
                mixerLineControls.cbmxctrl = sizeof(MIXERCONTROL);
                if (mixerGetLineControls(reinterpret_cast<HMIXEROBJ>(mixerHandle),
                        &mixerLineControls, MIXER_GETLINECONTROLSF_ONEBYTYPE) !=
                        MMSYSERR_NOERROR) {
                    LOG(LogError) <<
                            "VolumeControl::getVolume() - Failed to get mixer volume control!";
                    mixerClose(mixerHandle);
                    mixerHandle = nullptr;
                }
            }
            else {
                LOG(LogError) << "VolumeControl::init() - Failed to open mixer!";
            }
        }
    }
    else {
        // Windows Vista or above. use EndpointVolume API. get device enumerator.
        if (endpointVolume == nullptr) {
            CoInitialize(nullptr);
            IMMDeviceEnumerator * deviceEnumerator = nullptr;
            CoCreateInstance(__uuidof(MMDeviceEnumerator), nullptr, CLSCTX_INPROC_SERVER,
                    __uuidof(IMMDeviceEnumerator), reinterpret_cast<LPVOID *>(&deviceEnumerator));
            if (deviceEnumerator != nullptr) {
                // Get default endpoint.
                IMMDevice * defaultDevice = nullptr;
                deviceEnumerator->GetDefaultAudioEndpoint(eRender, eConsole, &defaultDevice);
                if (defaultDevice != nullptr) {
                    // Retrieve endpoint volume.
                    defaultDevice->Activate(__uuidof(IAudioEndpointVolume),
                            CLSCTX_INPROC_SERVER, nullptr,
                            reinterpret_cast<LPVOID *>(&endpointVolume));
                    if (endpointVolume == nullptr)
                        LOG(LogError) << "VolumeControl::init() - "
                                "Failed to get default audio endpoint volume!";
                    // Release default device. we don't need it anymore.
                    defaultDevice->Release();
                }
                else {
                    LOG(LogError) <<
                            "VolumeControl::init() - Failed to get default audio endpoint!";
                }
                // Release device enumerator. we don't need it anymore.
                deviceEnumerator->Release();
            }
            else {
                LOG(LogError) << "VolumeControl::init() - Failed to get audio endpoint enumerator!";
                CoUninitialize();
            }
        }
    }
    #endif
}

void VolumeControl::deinit()
{
    // Deinitialize audio mixer interface.
    #if defined(__APPLE__)
//    #error TODO: Not implemented for MacOS yet!!!
    #elif defined(__linux__)
    if (mixerHandle != nullptr) {
        snd_mixer_detach(mixerHandle, mixerCard.c_str());
        snd_mixer_free(mixerHandle);
        snd_mixer_close(mixerHandle);
        mixerHandle = nullptr;
        mixerElem = nullptr;
    }
    #elif defined(_WIN64)
    if (mixerHandle != nullptr) {
        mixerClose(mixerHandle);
        mixerHandle = nullptr;
    }
    else if (endpointVolume != nullptr) {
        endpointVolume->Release();
        endpointVolume = nullptr;
        CoUninitialize();
    }
    #endif
}

int VolumeControl::getVolume() const
{
    int volume = 0;

    #if defined(__APPLE__)
//    #error TODO: Not implemented for MacOS yet!!!
    #elif defined(__linux__)
    if (mixerElem != nullptr) {
        // Get volume range.
        long minVolume;
        long maxVolume;
        if (snd_mixer_selem_get_playback_volume_range(mixerElem, &minVolume, &maxVolume) == 0) {
            // Ok, now get volume.
            long rawVolume;
            if (snd_mixer_selem_get_playback_volume(mixerElem,
                    SND_MIXER_SCHN_MONO, &rawVolume) == 0) {
                // Worked. bring into range 0-100.
                rawVolume -= minVolume;
                if (rawVolume > 0)
                    volume = (rawVolume * 100.0) / (maxVolume - minVolume) + 0.5;
                //else
                //   volume = 0;
            }
            else {
                LOG(LogError) << "VolumeControl::getVolume() - Failed to get mixer volume!";
            }
        }
        else {
            LOG(LogError) << "VolumeControl::getVolume() - Failed to get volume range!";
        }
    }
    #elif defined(_WIN64)
    if (mixerHandle != nullptr) {
        // Windows older than Vista. use mixer API. get volume from line control.
        MIXERCONTROLDETAILS_UNSIGNED value;
        MIXERCONTROLDETAILS mixerControlDetails;
        mixerControlDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
        mixerControlDetails.dwControlID = mixerControl.dwControlID;
        // Always 1 for a MIXERCONTROL_CONTROLF_UNIFORM control.
        mixerControlDetails.cChannels = 1;
        // Always 0 except for a MIXERCONTROL_CONTROLF_MULTIPLE control.
        mixerControlDetails.cMultipleItems = 0;
        mixerControlDetails.paDetails = &value;
        mixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
        if (mixerGetControlDetails(reinterpret_cast<HMIXEROBJ>(mixerHandle), &mixerControlDetails,
                MIXER_GETCONTROLDETAILSF_VALUE) == MMSYSERR_NOERROR)
            volume = static_cast<int>(Math::round((value.dwValue * 100) / 65535.0f));
        else
            LOG(LogError) << "VolumeControl::getVolume() - Failed to get mixer volume!";
    }
    else if (endpointVolume != nullptr) {
        // Windows Vista or above. use EndpointVolume API.
        float floatVolume = 0.0f; // 0-1
        if (endpointVolume->GetMasterVolumeLevelScalar(&floatVolume) == S_OK) {
            volume = static_cast<int>(Math::round(floatVolume * 100.0f));
            LOG(LogInfo) << "System audio volume is " << volume;
        }
        else {
            LOG(LogError) << "VolumeControl::getVolume() - Failed to get master volume!";
        }
    }
    #endif

    // Clamp to 0-100 range.
    if (volume < 0)
        volume = 0;
    if (volume > 100)
        volume = 100;
    return volume;
}

void VolumeControl::setVolume(int volume)
{
    // Clamp to 0-100 range.
    if (volume < 0)
        volume = 0;
    if (volume > 100)
        volume = 100;

    // Store values in internal variables.
    internalVolume = volume;
    #if defined(__APPLE__)
//    #error TODO: Not implemented for MacOS yet!!!
    #elif defined(__linux__)
    if (mixerElem != nullptr) {
        // Get volume range.
        long minVolume;
        long maxVolume;
        if (snd_mixer_selem_get_playback_volume_range(mixerElem, &minVolume, &maxVolume) == 0) {
            // Ok, bring into minVolume-maxVolume range and set.
            long rawVolume = (volume * (maxVolume - minVolume) / 100) + minVolume;
            if (snd_mixer_selem_set_playback_volume(mixerElem,
                    SND_MIXER_SCHN_FRONT_LEFT, rawVolume) < 0 ||
                    snd_mixer_selem_set_playback_volume(mixerElem,
                    SND_MIXER_SCHN_FRONT_RIGHT, rawVolume) < 0) {
                LOG(LogError) << "VolumeControl::getVolume() - Failed to set mixer volume!";
            }
        }
        else {
            LOG(LogError) << "VolumeControl::getVolume() - Failed to get volume range!";
        }
    }
    #elif defined(_WIN64)
    if (mixerHandle != nullptr) {
        // Windows older than Vista. use mixer API. get volume from line control.
        MIXERCONTROLDETAILS_UNSIGNED value;
        value.dwValue = (volume * 65535) / 100;
        MIXERCONTROLDETAILS mixerControlDetails;
        mixerControlDetails.cbStruct = sizeof(MIXERCONTROLDETAILS);
        mixerControlDetails.dwControlID = mixerControl.dwControlID;
        // Always 1 for a MIXERCONTROL_CONTROLF_UNIFORM control.
        mixerControlDetails.cChannels = 1;
        // Always 0 except for a MIXERCONTROL_CONTROLF_MULTIPLE control.
        mixerControlDetails.cMultipleItems = 0;
        mixerControlDetails.paDetails = &value;
        mixerControlDetails.cbDetails = sizeof(MIXERCONTROLDETAILS_UNSIGNED);
        if (mixerSetControlDetails(reinterpret_cast<HMIXEROBJ>(mixerHandle), &mixerControlDetails,
                MIXER_SETCONTROLDETAILSF_VALUE) != MMSYSERR_NOERROR)
            LOG(LogError) << "VolumeControl::setVolume() - Failed to set mixer volume!";
    }
    else if (endpointVolume != nullptr) {
        // Windows Vista or above. use EndpointVolume API.
        float floatVolume = 0.0f; // 0-1
        if (volume > 0)
            floatVolume = static_cast<float>(volume) / 100.0f;
        if (endpointVolume->SetMasterVolumeLevelScalar(floatVolume, nullptr) != S_OK)
            LOG(LogError) << "VolumeControl::setVolume() - Failed to set master volume!";
    }
    #endif
}
