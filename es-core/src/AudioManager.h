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
#include <atomic>
#include <memory>
#include <vector>

class Sound;

class AudioManager
{
public:
    virtual ~AudioManager();
    static AudioManager& getInstance();

    void init();
    void deinit();

    void registerSound(std::shared_ptr<Sound> sound);
    void unregisterSound(std::shared_ptr<Sound> sound);

    void play();
    void stop();

    // Used for streaming audio from videos.
    void setupAudioStream(int sampleRate);
    void processStream(const void* samples, unsigned count);
    void clearStream();

    void muteStream() { sMuteStream = true; }
    void unmuteStream() { sMuteStream = false; }

    bool getHasAudioDevice() { return sHasAudioDevice; }

    static inline SDL_AudioDeviceID sAudioDevice {0};
    static inline SDL_AudioSpec sAudioFormat;

private:
    AudioManager() noexcept;

    static void mixAudio(void* unused, Uint8* stream, int len);

    static inline SDL_AudioStream* sConversionStream;
    static inline std::vector<std::shared_ptr<Sound>> sSoundVector;
    static inline std::atomic<bool> sMuteStream = false;
    static inline bool sHasAudioDevice = true;
};

#endif // ES_CORE_AUDIO_MANAGER_H
