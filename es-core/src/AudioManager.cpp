//
//  AudioManager.cpp
//
//  Low-level audio functions (using SDL2).
//

#include "AudioManager.h"

#include "Log.h"
#include "Settings.h"
#include "Sound.h"

#if defined(__linux__) || defined(_WIN64)
#include <SDL2/SDL.h>
#else
#include "SDL.h"
#endif

std::vector<std::shared_ptr<Sound>> AudioManager::sSoundVector;
SDL_AudioSpec AudioManager::sAudioFormat;
std::shared_ptr<AudioManager> AudioManager::sInstance;

void AudioManager::mixAudio(void* /*unused*/, Uint8 *stream, int len)
{
    bool stillPlaying = false;

    // Initialize the buffer to "silence".
    SDL_memset(stream, 0, len);

    // Iterate through all our samples.
    std::vector<std::shared_ptr<Sound>>::const_iterator soundIt = sSoundVector.cbegin();
    while (soundIt != sSoundVector.cend()) {
        std::shared_ptr<Sound> sound = *soundIt;
        if(sound->isPlaying()) {
            // Calculate rest length of current sample.
            Uint32 restLength = (sound->getLength() - sound->getPosition());
            if (restLength > (Uint32)len) {
                // If stream length is smaller than sample length, clip it.
                restLength = len;
            }
            // Mix sample into stream.
            SDL_MixAudio(stream, &(sound->getData()[sound->getPosition()]),
                    restLength, SDL_MIX_MAXVOLUME);
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
        // No. pause audio till a Sound::play() wakes us up.
        SDL_PauseAudio(1);
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

std::shared_ptr<AudioManager> & AudioManager::getInstance()
{
    // Check if an AudioManager instance is already created, if not create one.
    if (sInstance == nullptr && Settings::getInstance()->getBool("NavigationSounds")) {
        sInstance = std::shared_ptr<AudioManager>(new AudioManager);
    }
    return sInstance;
}

void AudioManager::init()
{
    if (SDL_InitSubSystem(SDL_INIT_AUDIO) != 0) {
        LOG(LogError) << "Error initializing SDL audio!\n" << SDL_GetError();
        return;
    }

    // Stop playing all Sounds.
    for(unsigned int i = 0; i < sSoundVector.size(); i++) {
        if(sSoundVector.at(i)->isPlaying())
            sSoundVector[i]->stop();
    }

    // Set up format and callback. Play 16-bit stereo audio at 44.1Khz.
    sAudioFormat.freq = 44100;
    sAudioFormat.format = AUDIO_S16;
    sAudioFormat.channels = 2;
    sAudioFormat.samples = 4096;
    sAudioFormat.callback = mixAudio;
    sAudioFormat.userdata = nullptr;

    // Open the audio device and pause.
    if (SDL_OpenAudio(&sAudioFormat, nullptr) < 0) {
        LOG(LogError) << "AudioManager Error - Unable to open SDL audio: " <<
                SDL_GetError() << std::endl;
    }
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

void AudioManager::registerSound(std::shared_ptr<Sound> & sound)
{
    getInstance();
    sSoundVector.push_back(sound);
}

void AudioManager::unregisterSound(std::shared_ptr<Sound> & sound)
{
    getInstance();
    for(unsigned int i = 0; i < sSoundVector.size(); i++) {
        if(sSoundVector.at(i) == sound) {
            sSoundVector[i]->stop();
            sSoundVector.erase(sSoundVector.cbegin() + i);
            return;
        }
    }
    LOG(LogError) << "AudioManager Error - tried to unregister a sound that wasn't registered!";
}

void AudioManager::play()
{
    getInstance();

    // Unpause audio, the mixer will figure out if samples need to be played...
    SDL_PauseAudio(0);
}

void AudioManager::stop()
{
    // Stop playing all Sounds.
    for(unsigned int i = 0; i < sSoundVector.size(); i++) {
        if(sSoundVector.at(i)->isPlaying())
            sSoundVector[i]->stop();
    }
    // Pause audio
    SDL_PauseAudio(1);
}
