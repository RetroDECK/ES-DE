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

std::shared_ptr<AudioManager> AudioManager::sInstance;
std::vector<std::shared_ptr<Sound>> AudioManager::sSoundVector;
SDL_AudioDeviceID AudioManager::sAudioDevice = 0;
SDL_AudioSpec AudioManager::sAudioFormat;

void AudioManager::mixAudio(void* /*unused*/, Uint8* stream, int len)
{
    bool stillPlaying = false;

    // Initialize the buffer to "silence".
    SDL_memset(stream, 0, len);

    // Iterate through all our samples.
    std::vector<std::shared_ptr<Sound>>::const_iterator soundIt = sSoundVector.cbegin();
    while (soundIt != sSoundVector.cend()) {
        std::shared_ptr<Sound> sound = *soundIt;
        if (sound->isPlaying()) {
            // Calculate rest length of current sample.
            Uint32 restLength = (sound->getLength() - sound->getPosition());
            if (restLength > static_cast<Uint32>(len)) {
                // If stream length is smaller than sample length, clip it.
                restLength = len;
            }
            // Mix sample into stream.
            SDL_MixAudioFormat(stream, &(sound->getData()[sound->getPosition()]), AUDIO_S16,
                    restLength, Settings::getInstance()->getInt("SoundVolumeNavigation") * 1.28);
            if (sound->getPosition() + restLength < sound->getLength()) {
                //sample hasn't ended yet
                stillPlaying = true;
            }
            // Set new sound position. if this is at or beyond the end of the sample,
            // it will stop automatically.
            sound->setPosition(sound->getPosition() + restLength);
        }
        // Advance to next sound.
        ++soundIt;
    }

    // We have processed all samples. check if some will still be playing.
    if (!stillPlaying) {
        // Nothing is playing, pause the audio until Sound::play() wakes us up.
        SDL_PauseAudioDevice(sAudioDevice, 1);
    }
}

AudioManager::AudioManager()
{
    init();
}

AudioManager::~AudioManager()
{
    deinit();
}

std::shared_ptr<AudioManager>& AudioManager::getInstance()
{
    // Check if an AudioManager instance is already created, if not create it.
    if (sInstance == nullptr)
        sInstance = std::shared_ptr<AudioManager>(new AudioManager);

    return sInstance;
}

void AudioManager::init()
{
    LOG(LogInfo) << "Setting up AudioManager...";

    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        LOG(LogError) << "Error initializing SDL audio!\n" << SDL_GetError();
        return;
    }

    // Stop playing all Sounds.
    for (unsigned int i = 0; i < sSoundVector.size(); i++) {
        if (sSoundVector.at(i)->isPlaying())
            sSoundVector[i]->stop();
    }

    SDL_AudioSpec sRequestedAudioFormat;

    SDL_memset(&sRequestedAudioFormat, 0, sizeof(sRequestedAudioFormat));
    SDL_memset(&sAudioFormat, 0, sizeof(sAudioFormat));

    // Set up format and callback. Play 16-bit stereo audio at 44.1Khz.
    sRequestedAudioFormat.freq = 44100;
    sRequestedAudioFormat.format = AUDIO_S16;
    sRequestedAudioFormat.channels = 2;
    sRequestedAudioFormat.samples = 4096;
    sRequestedAudioFormat.callback = mixAudio;
    sRequestedAudioFormat.userdata = nullptr;

    for (unsigned int i = 0; i < SDL_GetNumAudioDevices(0); i++) {
        LOG(LogDebug) << "Detected playback device '" << SDL_GetAudioDeviceName(i, 0) << "'.";
    }

    sAudioDevice = SDL_OpenAudioDevice(0, 0, &sRequestedAudioFormat, &sAudioFormat,
            SDL_AUDIO_ALLOW_ANY_CHANGE);

    if (sAudioDevice == 0) {
        LOG(LogError) << "Unable to open audio device: " << SDL_GetError() << std::endl;
        return;
    }

    if (sAudioFormat.freq != sRequestedAudioFormat.freq) {
        LOG(LogDebug) << "AudioManager::init(): Requested frequency 44100 could not be "
                "set, obtained " << std::to_string(sAudioFormat.freq) << ".";
    }
    if (sAudioFormat.format != sRequestedAudioFormat.format) {
        LOG(LogDebug) << "AudioManager::init(): Requested format " << AUDIO_S16 << " could not be "
                "set, obtained " << std::to_string(sAudioFormat.format) << ".";
    }
    if (sAudioFormat.channels != sRequestedAudioFormat.channels) {
        LOG(LogDebug) << "AudioManager::init(): Requested channel count 2 could not be "
                "set, obtained " << std::to_string(sAudioFormat.channels) << ".";
    }
    if (sAudioFormat.samples != sRequestedAudioFormat.samples) {
        LOG(LogDebug) << "AudioManager::init(): Requested sample buffer size 4096 could not be "
                "set, obtained " << std::to_string(sAudioFormat.samples) << ".";
    }

    // Just in case someone changed the es_settings.cfg file manually to invalid values.
    if (Settings::getInstance()->getInt("SoundVolumeNavigation") > 100)
        Settings::getInstance()->setInt("SoundVolumeNavigation", 100);
    if (Settings::getInstance()->getInt("SoundVolumeNavigation") < 0)
        Settings::getInstance()->setInt("SoundVolumeNavigation", 0);
    if (Settings::getInstance()->getInt("SoundVolumeVideos") > 100)
        Settings::getInstance()->setInt("SoundVolumeVideos", 100);
    if (Settings::getInstance()->getInt("SoundVolumeVideos") < 0)
        Settings::getInstance()->setInt("SoundVolumeVideos", 0);
}

void AudioManager::deinit()
{
    // Stop all playback.
    stop();
    // Completely tear down SDL audio. else SDL hogs audio resources and
    // emulators might fail to start...
    SDL_CloseAudio();
    SDL_QuitSubSystem(SDL_INIT_AUDIO);
    sInstance = nullptr;
}

void AudioManager::registerSound(std::shared_ptr<Sound>& sound)
{
    sSoundVector.push_back(sound);
}

void AudioManager::unregisterSound(std::shared_ptr<Sound>& sound)
{
    for (unsigned int i = 0; i < sSoundVector.size(); i++) {
        if (sSoundVector.at(i) == sound) {
            sSoundVector[i]->stop();
            sSoundVector.erase(sSoundVector.cbegin() + i);
            return;
        }
    }
    LOG(LogError) << "AudioManager - tried to unregister a sound that wasn't registered!";
}

void AudioManager::play()
{
    // Unpause audio, the mixer will figure out if samples need to be played...
    SDL_PauseAudioDevice(sAudioDevice, 0);
}

void AudioManager::stop()
{
    // Stop playing all Sounds.
    for (unsigned int i = 0; i < sSoundVector.size(); i++) {
        if (sSoundVector.at(i)->isPlaying())
            sSoundVector[i]->stop();
    }
    // Pause audio.
    SDL_PauseAudioDevice(sAudioDevice, 1);
}
