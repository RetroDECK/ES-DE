//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  AudioManager.cpp
//
//  Low-level audio functions (using SDL2).
//

#include "AudioManager.h"

#include "Log.h"
#include "Settings.h"
#include "Sound.h"

#include <SDL2/SDL.h>

AudioManager::AudioManager() noexcept
{
    // Init on construction.
    init();
}

AudioManager::~AudioManager()
{
    // Deinit on destruction.
    deinit();
}

AudioManager& AudioManager::getInstance()
{
    static AudioManager instance;
    return instance;
}

void AudioManager::init()
{
    LOG(LogInfo) << "Setting up AudioManager...";

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        LOG(LogError) << "Error initializing SDL audio!\n" << SDL_GetError();
        return;
    }

    LOG(LogInfo) << "Audio driver: " << SDL_GetCurrentAudioDriver();

    SDL_AudioSpec sRequestedAudioFormat;

    SDL_memset(&sRequestedAudioFormat, 0, sizeof(sRequestedAudioFormat));
    SDL_memset(&sAudioFormat, 0, sizeof(sAudioFormat));

    // Set up format and callback. SDL will negotiate these settings with the audio driver, so
    // if for instance the driver/hardware does not support 32-bit floating point output, 16-bit
    // integer may be selected instead. ES-DE will handle this automatically as there are no
    // hardcoded audio settings elsewhere in the code.
    sRequestedAudioFormat.freq = 44100;
    sRequestedAudioFormat.format = AUDIO_F32;
    sRequestedAudioFormat.channels = 2;
    sRequestedAudioFormat.samples = 1024;
    sRequestedAudioFormat.callback = mixAudio;
    sRequestedAudioFormat.userdata = nullptr;

    for (int i {0}; i < SDL_GetNumAudioDevices(0); ++i) {
        LOG(LogInfo) << "Detected playback device: " << SDL_GetAudioDeviceName(i, 0);
    }

    sAudioDevice = SDL_OpenAudioDevice(0, 0, &sRequestedAudioFormat, &sAudioFormat,
                                       SDL_AUDIO_ALLOW_ANY_CHANGE);

    if (sAudioDevice == 0) {
        LOG(LogError) << "Unable to open audio device: " << SDL_GetError();
        sHasAudioDevice = false;
    }

    if (sAudioFormat.freq != sRequestedAudioFormat.freq) {
        LOG(LogDebug) << "AudioManager::init(): Requested sample rate "
                      << std::to_string(sRequestedAudioFormat.freq)
                      << " could not be set, obtained " << std::to_string(sAudioFormat.freq);
    }
    if (sAudioFormat.format != sRequestedAudioFormat.format) {
        LOG(LogDebug) << "AudioManager::init(): Requested format "
                      << std::to_string(sRequestedAudioFormat.format)
                      << " could not be set, obtained " << std::to_string(sAudioFormat.format);
    }
    if (sAudioFormat.channels != sRequestedAudioFormat.channels) {
        LOG(LogDebug) << "AudioManager::init(): Requested channel count "
                      << std::to_string(sRequestedAudioFormat.channels)
                      << " could not be set, obtained " << std::to_string(sAudioFormat.channels);
    }
#if defined(_WIN64) || defined(__APPLE__)
    // Beats me why the buffer size is not divided by the channel count on some operating systems.
    if (sAudioFormat.samples != sRequestedAudioFormat.samples) {
#else
    if (sAudioFormat.samples != sRequestedAudioFormat.samples / sRequestedAudioFormat.channels) {
#endif
        LOG(LogDebug) << "AudioManager::init(): Requested sample buffer size "
                      << std::to_string(sRequestedAudioFormat.samples /
                                        sRequestedAudioFormat.channels)
                      << " could not be set, obtained " << std::to_string(sAudioFormat.samples);
    }

    // Just in case someone changed the es_settings.xml file manually to invalid values.
    if (Settings::getInstance()->getInt("SoundVolumeNavigation") > 100)
        Settings::getInstance()->setInt("SoundVolumeNavigation", 100);
    if (Settings::getInstance()->getInt("SoundVolumeNavigation") < 0)
        Settings::getInstance()->setInt("SoundVolumeNavigation", 0);
    if (Settings::getInstance()->getInt("SoundVolumeVideos") > 100)
        Settings::getInstance()->setInt("SoundVolumeVideos", 100);
    if (Settings::getInstance()->getInt("SoundVolumeVideos") < 0)
        Settings::getInstance()->setInt("SoundVolumeVideos", 0);

    setupAudioStream(sRequestedAudioFormat.freq);
}

void AudioManager::deinit()
{
    SDL_LockAudioDevice(sAudioDevice);
    SDL_FreeAudioStream(sConversionStream);
    SDL_UnlockAudioDevice(sAudioDevice);

    SDL_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);

    sAudioDevice = 0;
}

void AudioManager::mixAudio(void* /*unused*/, Uint8* stream, int len)
{
    // Process navigation sounds.
    bool stillPlaying {false};

    // Initialize the buffer to "silence".
    SDL_memset(stream, 0, len);

    // Iterate through all our samples.
    std::vector<std::shared_ptr<Sound>>::const_iterator soundIt = sSoundVector.cbegin();
    while (soundIt != sSoundVector.cend()) {
        std::shared_ptr<Sound> sound {*soundIt};
        if (sound->isPlaying()) {
            // Calculate rest length of current sample.
            Uint32 restLength {sound->getLength() - sound->getPosition()};
            if (restLength > static_cast<Uint32>(len)) {
                // If stream length is smaller than sample length, clip it.
                restLength = len;
            }
            // Mix sample into stream.
            SDL_MixAudioFormat(
                stream, &(sound->getData()[sound->getPosition()]), sAudioFormat.format, restLength,
                static_cast<int>(Settings::getInstance()->getInt("SoundVolumeNavigation") * 1.28f));
            if (sound->getPosition() + restLength < sound->getLength()) {
                // Sample hasn't ended yet.
                stillPlaying = true;
            }
            // Set new sound position. if this is at or beyond the end of the sample,
            // it will stop automatically.
            sound->setPosition(sound->getPosition() + restLength);
        }
        // Advance to next sound.
        ++soundIt;
    }

    // Process video stream audio generated by VideoFFmpegComponent.
    int streamLength {SDL_AudioStreamAvailable(sConversionStream)};

    if (streamLength <= 0) {
        // If nothing is playing, pause the device until there is more audio to output.
        if (!stillPlaying)
            SDL_PauseAudioDevice(sAudioDevice, 1);
        return;
    }

    int chunkLength {0};

    // Cap the chunk length to the buffer size.
    if (streamLength > len)
        chunkLength = len;
    else
        chunkLength = streamLength;

    std::vector<Uint8> converted(chunkLength);

    int processedLength {
        SDL_AudioStreamGet(sConversionStream, static_cast<void*>(&converted.at(0)), chunkLength)};

    if (processedLength < 0) {
        LOG(LogError) << "AudioManager::mixAudio(): Couldn't convert sound chunk:";
        LOG(LogError) << SDL_GetError();
        return;
    }

    // Enable only when needed, as this generates a lot of debug output.
    //    LOG(LogDebug) << "AudioManager::mixAudio(): chunkLength "
    //            "/ processedLength / streamLength: " << chunkLength << " / " <<
    //            " / " << processedLength << " / " << streamLength;

    // This mute flag is used to make sure that the audio buffer already sent to the
    // stream is not played when the video player has been stopped. Otherwise there would
    // be a short time period when the audio would keep playing after the video was stopped
    // and before the stream was cleared in clearStream().
    bool muteStream {sMuteStream};
    if (muteStream) {
        SDL_MixAudioFormat(stream, &converted.at(0), sAudioFormat.format, processedLength, 0);
    }
    else {
        SDL_MixAudioFormat(
            stream, &converted.at(0), sAudioFormat.format, processedLength,
            static_cast<int>(Settings::getInstance()->getInt("SoundVolumeVideos") * 1.28f));
    }

    // If nothing is playing, pause the device until there is more audio to output.
    if (!stillPlaying && SDL_AudioStreamAvailable(sConversionStream) == 0)
        SDL_PauseAudioDevice(sAudioDevice, 1);
}

void AudioManager::registerSound(std::shared_ptr<Sound> sound)
{
    // Add sound to sound vector.
    sSoundVector.push_back(sound);
}

void AudioManager::unregisterSound(std::shared_ptr<Sound> sound)
{
    for (unsigned int i {0}; i < sSoundVector.size(); ++i) {
        if (sSoundVector.at(i) == sound) {
            sSoundVector[i]->stop();
            sSoundVector.erase(sSoundVector.cbegin() + i);
            return;
        }
    }
}

void AudioManager::play()
{
    // Unpause audio, the mixer will figure out if samples need to be played...
    SDL_PauseAudioDevice(sAudioDevice, 0);
}

void AudioManager::stop()
{
    // Stop playing all Sounds.
    for (unsigned int i {0}; i < sSoundVector.size(); ++i) {
        if (sSoundVector.at(i)->isPlaying())
            sSoundVector[i]->stop();
    }
    // Pause audio.
    SDL_PauseAudioDevice(sAudioDevice, 1);
}

void AudioManager::setupAudioStream(int sampleRate)
{
    SDL_AudioStatus audioStatus {SDL_GetAudioDeviceStatus(sAudioDevice)};

    // It's very important to pause the audio device before setting up the stream,
    // or we may get random crashes if attempting to play samples at the same time.
    SDL_PauseAudioDevice(sAudioDevice, 1);
    SDL_FreeAudioStream(sConversionStream);

    // Used for streaming audio from videos.
    sConversionStream = SDL_NewAudioStream(AUDIO_F32, 2, sampleRate, sAudioFormat.format,
                                           sAudioFormat.channels, sAudioFormat.freq);
    if (sConversionStream == nullptr) {
        LOG(LogError) << "Failed to create audio conversion stream:";
        LOG(LogError) << SDL_GetError();
    }

    // If the device was previously in a playing state, then restore it.
    if (audioStatus == SDL_AUDIO_PLAYING)
        SDL_PauseAudioDevice(sAudioDevice, 0);
}

void AudioManager::processStream(const void* samples, unsigned count)
{
    SDL_LockAudioDevice(sAudioDevice);

    if (SDL_AudioStreamPut(sConversionStream, samples, count * sizeof(Uint8)) == -1) {
        LOG(LogError) << "Failed to put samples in the conversion stream:";
        LOG(LogError) << SDL_GetError();
        SDL_UnlockAudioDevice(sAudioDevice);
        return;
    }

    if (count > 0)
        SDL_PauseAudioDevice(sAudioDevice, 0);

    SDL_UnlockAudioDevice(sAudioDevice);
}

void AudioManager::clearStream()
{
    SDL_LockAudioDevice(sAudioDevice);
    SDL_AudioStreamClear(sConversionStream);
    SDL_UnlockAudioDevice(sAudioDevice);
}
