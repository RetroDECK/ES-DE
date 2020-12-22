//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  AudioManager.h
//
//  Low-level audio functions (using SDL2).
//

#ifndef ES_CORE_AUDIO_MANAGER_H
#define ES_CORE_AUDIO_MANAGER_H

#include <SDL2/SDL_audio.h>
#include <memory>
#include <vector>

class Sound;

class AudioManager
{
public:
    virtual ~AudioManager();
    static std::shared_ptr<AudioManager>& getInstance();

    void init();
    void deinit();

    void registerSound(std::shared_ptr<Sound>& sound);
    void unregisterSound(std::shared_ptr<Sound>& sound);

    void play();
    void stop();

    // Used for streaming audio from videos.
    void processStream(const void *samples, unsigned count);

    static SDL_AudioDeviceID sAudioDevice;
    static SDL_AudioSpec sAudioFormat;

private:
    AudioManager();

    static void mixAudio(void* unused, Uint8* stream, int len);
    static void mixAudio2(void* unused, Uint8* stream, int len);
    static SDL_AudioStream* sConversionStream;
    static std::vector<std::shared_ptr<Sound>> sSoundVector;
    static std::shared_ptr<AudioManager> sInstance;
};

#endif // ES_CORE_AUDIO_MANAGER_H
