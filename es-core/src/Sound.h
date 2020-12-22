//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  Sound.h
//
//  Higher-level audio functions.
//  Reading theme sound setings, playing audio samples etc.
//

#ifndef ES_CORE_SOUND_H
#define ES_CORE_SOUND_H

#include <SDL2/SDL_audio.h>
#include <map>
#include <memory>
#include <sstream>
#include <string>
#include <vector>

class ThemeData;

class Sound
{
public:
    ~Sound();

    void init();
    void deinit();

    void loadFile(const std::string& path);

    void play();
    bool isPlaying() const;
    void stop();

    const Uint8* getData() const;
    Uint32 getPosition() const;
    void setPosition(Uint32 newPosition);
    Uint32 getLength() const;

    static std::shared_ptr<Sound> get(const std::string& path);
    static std::shared_ptr<Sound> getFromTheme(const std::shared_ptr<ThemeData>& theme,
            const std::string& view, const std::string& elem);

private:
    Sound(const std::string& path = "");

    static std::map<std::string, std::shared_ptr<Sound>> sMap;
    std::string mPath;
    SDL_AudioSpec mSampleFormat;
    Uint8* mSampleData;
    Uint32 mSamplePos;
    Uint32 mSampleLength;
    bool playing;
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
    static NavigationSounds* getInstance();

    void deinit();
    void loadThemeNavigationSounds(const std::shared_ptr<ThemeData>& theme);
    void playThemeNavigationSound(NavigationSoundsID soundID);
    bool isPlayingThemeNavigationSound(NavigationSoundsID soundID);

private:
    static NavigationSounds* sInstance;
    std::vector<std::shared_ptr<Sound>> navigationSounds;
};

#endif // ES_CORE_SOUND_H
