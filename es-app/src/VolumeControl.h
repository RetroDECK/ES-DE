//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  VolumeControl.h
//
//  Controls system audio volume.
//

#ifndef ES_APP_VOLUME_CONTROL_H
#define ES_APP_VOLUME_CONTROL_H

#include <memory>

#if defined(__APPLE__)
//#error TODO: Not implemented for MacOS yet!!!
#elif defined(__linux__)
#include <alsa/asoundlib.h>
#include <fcntl.h>
#include <unistd.h>
#elif defined(_WIN64)
#include <Windows.h>
#include <endpointvolume.h>
#include <mmdeviceapi.h>
#include <mmsystem.h>
#endif

//  Singleton pattern. Call getInstance() to get an object.
class VolumeControl
{
    #if defined(__APPLE__)
//    #error TODO: Not implemented for MacOS yet!!!
    #elif defined(__linux__)
    static std::string mixerName;
    static std::string mixerCard;
    int mixerIndex;
    snd_mixer_t* mixerHandle;
    snd_mixer_elem_t* mixerElem;
    snd_mixer_selem_id_t* mixerSelemId;
    #elif defined(_WIN64)
    HMIXER mixerHandle;
    MIXERCONTROL mixerControl;
    IAudioEndpointVolume * endpointVolume;
    #endif

    int originalVolume;
    int internalVolume;

    static std::weak_ptr<VolumeControl> sInstance;

    VolumeControl();
    VolumeControl(const VolumeControl & right);
    VolumeControl & operator=(const VolumeControl & right);

public:
    static std::shared_ptr<VolumeControl> & getInstance();

    void init();
    void deinit();

    int getVolume() const;
    void setVolume(int volume);

    ~VolumeControl();
};

#endif // ES_APP_VOLUME_CONTROL_H
