//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Sound.h
//
//  Higher-level audio functions.
//  Navigation sounds, audio sample playback etc.
//

#ifndef ES_CORE_SOUND_H
#define ES_CORE_SOUND_H

#include <SDL2/SDL_audio.h>
#include <atomic>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

class ThemeData;

class Sound
{
public:
    ~Sound() {}

    void init();
    void deinit();

    void loadFile(const std::string& path);

    void play();
    bool isPlaying() const { return mPlaying; }
    void stop();

    const Uint8* getData() const { return mSampleData; }
    Uint32 getPosition() const { return mSamplePos; }
    void setPosition(Uint32 newPosition);
    Uint32 getLength() const { return mSampleLength; }

    static std::shared_ptr<Sound> get(const std::string& path);
    static std::shared_ptr<Sound> getFromTheme(ThemeData* const theme,
                                               const std::string& view,
                                               const std::string& elem);

private:
    Sound(const std::string& path = "");

    static inline std::map<std::string, std::shared_ptr<Sound>> sMap;
    std::string mPath;
    SDL_AudioSpec mSampleFormat;
    Uint8* mSampleData;
    Uint32 mSamplePos;
    Uint32 mSampleLength;
    std::atomic<bool> mPlaying;
};

enum NavigationSoundsID {
    SYSTEMBROWSESOUND,
    QUICKSYSSELECTSOUND,
    SELECTSOUND,
    BACKSOUND,
    SCROLLSOUND,
    FAVORITESOUND,
    LAUNCHSOUND
};

class NavigationSounds
{
public:
    static NavigationSounds& getInstance();

    void deinit();
    void loadThemeNavigationSounds(ThemeData* const theme);
    void playThemeNavigationSound(NavigationSoundsID soundID);
    bool isPlayingThemeNavigationSound(NavigationSoundsID soundID);

private:
    NavigationSounds() noexcept {};
    std::vector<std::shared_ptr<Sound>> mNavigationSounds;
};

#endif // ES_CORE_SOUND_H
