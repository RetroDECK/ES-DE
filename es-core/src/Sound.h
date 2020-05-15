#pragma once
#ifndef ES_CORE_SOUND_H
#define ES_CORE_SOUND_H

#include "SDL_audio.h"
#include <map>
#include <memory>

class ThemeData;

class Sound
{
	std::string mPath;
    SDL_AudioSpec mSampleFormat;
	Uint8 * mSampleData;
    Uint32 mSamplePos;
    Uint32 mSampleLength;
	bool playing;

public:
	static std::shared_ptr<Sound> get(const std::string& path);
	static std::shared_ptr<Sound> getFromTheme(const std::shared_ptr<ThemeData>& theme, const std::string& view, const std::string& elem);

	~Sound();

	void init();
	void deinit();

	void loadFile(const std::string & path);

	void play();
	bool isPlaying() const;
	void stop();

	const Uint8 * getData() const;
	Uint32 getPosition() const;
	void setPosition(Uint32 newPosition);
	Uint32 getLength() const;
	Uint32 getLengthMS() const;

private:
	Sound(const std::string & path = "");
	static std::map< std::string, std::shared_ptr<Sound> > sMap;
};

enum NavigationSoundsID
{
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
	void loadThemeNavigationSounds(const std::shared_ptr<ThemeData>& theme);
	void playThemeNavigationSound(NavigationSoundsID soundID);
	bool isPlayingThemeNavigationSound(NavigationSoundsID soundID);

private:
	std::shared_ptr<Sound> systembrowseSound;
	std::shared_ptr<Sound> quicksysselectSound;
	std::shared_ptr<Sound> selectSound;
	std::shared_ptr<Sound> backSound;
	std::shared_ptr<Sound> scrollSound;
	std::shared_ptr<Sound> favoriteSound;
	std::shared_ptr<Sound> launchSound;
};

extern NavigationSounds navigationsounds;

#endif // ES_CORE_SOUND_H
