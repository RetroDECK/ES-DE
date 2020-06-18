//
//	FileData.cpp
//
//	Provides game file data structures and functions to access and sort this information.
//	Also provides functions to look up paths to media files and for launching games
//	(launching initiated by the ViewController).
//

#include "FileData.h"

#include "utils/FileSystemUtil.h"
#include "utils/StringUtil.h"
#include "utils/TimeUtil.h"
#include "AudioManager.h"
#include "CollectionSystemManager.h"
#include "FileFilterIndex.h"
#include "FileSorts.h"
#include "Log.h"
#include "MameNames.h"
#include "platform.h"
#include "Scripting.h"
#include "SystemData.h"
#include "VolumeControl.h"
#include "Window.h"
#include <assert.h>

FileData::FileData(
		FileType type,
		const std::string& path,
		SystemEnvironmentData* envData,
		SystemData* system)
		: mType(type),
		mPath(path),
		mSystem(system),
		mEnvData(envData),
		mSourceFileData(nullptr),
		mParent(nullptr),
		// Metadata is REALLY set in the constructor!
		metadata(type == GAME ? GAME_METADATA : FOLDER_METADATA)
{
	// Metadata needs at least a name field (since that's what getName() will return).
	if (metadata.get("name").empty()) {
		if ((system->hasPlatformId(PlatformIds::ARCADE) ||
				system->hasPlatformId(PlatformIds::NEOGEO)) &&
				metadata.getType() != FOLDER_METADATA) {
			// If it's a MAME or Neo Geo game, expand the game name accordingly.
			metadata.set("name",
					MameNames::getInstance()->getCleanName(getCleanName()));
		}
		else {
			metadata.set("name", getDisplayName());
		}
	}
	mSystemName = system->getName();
	metadata.resetChangedFlag();
}

FileData::~FileData()
{
	if (mParent)
		mParent->removeChild(this);

	if (mType == GAME)
		mSystem->getIndex()->removeFromIndex(this);

	mChildren.clear();
}

std::string FileData::getDisplayName() const
{
	std::string stem = Utils::FileSystem::getStem(mPath);
	return stem;
}

std::string FileData::getCleanName() const
{
	return Utils::String::removeParenthesis(this->getDisplayName());
}

const std::string& FileData::getName()
{
	return metadata.get("name");
}

const std::string& FileData::getSortName()
{
	if (metadata.get("sortname").empty())
		return metadata.get("name");
	else
		return metadata.get("sortname");
}

const bool FileData::getFavorite()
{
	if (metadata.get("favorite") == "true")
		return true;
	else
		return false;
}

const std::string FileData::getROMDirectory()
{
	std::string romDirSetting = Settings::getInstance()->getString("ROMDirectory");
	std::string romDirPath = "";

	if (romDirSetting == "") {
		romDirPath = Utils::FileSystem::getHomePath() + "/ROMs/";
	}
	else {
		romDirPath = romDirSetting;

		// Expand home symbol if the path starts with ~
		if (romDirPath[0] == '~') {
			romDirPath.erase(0, 1);
			romDirPath.insert(0, Utils::FileSystem::getHomePath());
		}
		if (romDirPath.back() !=  '/')
			romDirPath = romDirPath + "/";
	}

	return romDirPath;
}

const std::string FileData::getMediaDirectory()
{
	std::string mediaDirSetting = Settings::getInstance()->getString("MediaDirectory");
	std::string mediaDirPath = "";

	if (mediaDirSetting == "") {
		mediaDirPath = Utils::FileSystem::getHomePath() + "/.emulationstation/downloaded_media/";
	}
	else {
		mediaDirPath = mediaDirSetting;

		// Expand home symbol if the path starts with ~
		if (mediaDirPath[0] == '~') {
			mediaDirPath.erase(0, 1);
			mediaDirPath.insert(0, Utils::FileSystem::getHomePath());
		}
		if (mediaDirPath.back() !=  '/')
			mediaDirPath = mediaDirPath + "/";
	}

	return mediaDirPath;
}

const std::string FileData::getMediafilePath(std::string subdirectory, std::string mediatype) const
{
	const char* extList[2] = { ".png", ".jpg" };

	// Look for an image file in the media directory.
	std::string tempPath = getMediaDirectory() + mSystemName + "/" +
			subdirectory + "/" + getDisplayName();
	for (int i = 0; i < 2; i++) {
		std::string mediaPath = tempPath + extList[i];
		if (Utils::FileSystem::exists(mediaPath))
			return mediaPath;
	}

	// No media found in the media directory, so look
	// for local art as well (if configured to do so).
	if (Settings::getInstance()->getBool("LocalArt")) {
		for (int i = 0; i < 2; i++) {
			std::string localMediaPath = mEnvData->mStartPath + "/images/" +
					getDisplayName() + "-" + mediatype + extList[i];
			if (Utils::FileSystem::exists(localMediaPath))
				return localMediaPath;
		}
	}

	return "";
}

const std::string FileData::getImagePath() const
{
	// Look for a mix image (a combination of screenshot, 2D/3D box and marquee).
	std::string image = getMediafilePath("miximages", "miximage");
	if (image != "")
		return image;

	// If no mix image was found, try screenshot instead.
	image = getMediafilePath("screenshots", "screenshot");
	if (image != "")
		return image;

	// If no screenshot was found either, try cover.
	return getMediafilePath("covers", "cover");
}

const std::string FileData::get3DBoxPath() const
{
	return getMediafilePath("3dboxes", "3dbox");
}

const std::string FileData::getCoverPath() const
{
	return getMediafilePath("covers", "cover");
}

const std::string FileData::getMarqueePath() const
{
	return getMediafilePath("marquees", "marquee");
}

const std::string FileData::getMiximagePath() const
{
	return getMediafilePath("miximages", "miximage");
}

const std::string FileData::getScreenshotPath() const
{
	return getMediafilePath("screenshots", "screenshot");
}

const std::string FileData::getThumbnailPath() const
{
	return getMediafilePath("thumbnails", "thumbnail");
}

const std::string FileData::getVideoPath() const
{
	const char* extList[5] = { ".avi", ".mkv", ".mov", ".mp4", ".wmv" };
	std::string tempPath = getMediaDirectory() + mSystemName + "/videos/" + getDisplayName();

	// Look for media in the media directory.
	for (int i = 0; i < 5; i++) {
		std::string mediaPath = tempPath + extList[i];
		if (Utils::FileSystem::exists(mediaPath))
			return mediaPath;
	}

	// No media found in the media directory, so look
	// for local art as well (if configured to do so).
	if (Settings::getInstance()->getBool("LocalArt"))
	{
		for (int i = 0; i < 5; i++) {
			std::string localMediaPath = mEnvData->mStartPath + "/videos/" + getDisplayName() +
					"-video" + extList[i];
			if (Utils::FileSystem::exists(localMediaPath))
				return localMediaPath;
		}
	}

	return "";
}

const std::vector<FileData*>& FileData::getChildrenListToDisplay()
{

	FileFilterIndex* idx = CollectionSystemManager::get()->getSystemToView(mSystem)->getIndex();
	if (idx->isFiltered()) {
		mFilteredChildren.clear();
		for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++) {
			if (idx->showFile((*it))) {
				mFilteredChildren.push_back(*it);
			}
		}
		return mFilteredChildren;
	}
	else {
		return mChildren;
	}
}

std::vector<FileData*> FileData::getFilesRecursive(unsigned int typeMask, bool displayedOnly) const
{
	std::vector<FileData*> out;
	FileFilterIndex* idx = mSystem->getIndex();

	for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++) {
		if ((*it)->getType() & typeMask) {
			if (!displayedOnly || !idx->isFiltered() || idx->showFile(*it))
				out.push_back(*it);
		}
		if ((*it)->getChildren().size() > 0) {
			std::vector<FileData*> subchildren = (*it)->getFilesRecursive(typeMask, displayedOnly);
			out.insert(out.cend(), subchildren.cbegin(), subchildren.cend());
		}
	}

	return out;
}

std::string FileData::getKey() {
	return getFileName();
}

const bool FileData::isArcadeAsset()
{
	const std::string stem = Utils::FileSystem::getStem(mPath);
	return (
		(mSystem && (mSystem->hasPlatformId(PlatformIds::ARCADE) ||
				mSystem->hasPlatformId(PlatformIds::NEOGEO))) &&
				(MameNames::getInstance()->isBios(stem) ||
				MameNames::getInstance()->isDevice(stem))
	);
}

FileData* FileData::getSourceFileData()
{
	return this;
}

void FileData::addChild(FileData* file)
{
	assert(mType == FOLDER);
	assert(file->getParent() == NULL);

	const std::string key = file->getKey();
	if (mChildrenByFilename.find(key) == mChildrenByFilename.cend()) {
		mChildrenByFilename[key] = file;
		mChildren.push_back(file);
		file->mParent = this;
	}
}

void FileData::removeChild(FileData* file)
{
	assert(mType == FOLDER);
	assert(file->getParent() == this);
	mChildrenByFilename.erase(file->getKey());
	for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++) {
		if (*it == file) {
			file->mParent = NULL;
			mChildren.erase(it);
			return;
		}
	}

	// File somehow wasn't in our children.
	assert(false);
}

void FileData::sort(ComparisonFunction& comparator, bool ascending)
{
	mFirstLetterIndex.clear();
	std::stable_sort(mChildren.begin(), mChildren.end(), comparator);

	for (auto it = mChildren.cbegin(); it != mChildren.cend(); it++) {
		// Build mFirstLetterIndex.
		const char firstChar = toupper((*it)->getSortName().front());
		mFirstLetterIndex.push_back(std::string(1, firstChar));
		// Iterate through any child folders.
		if ((*it)->getChildren().size() > 0)
			(*it)->sort(comparator, ascending);
	}

	// Sort and make each entry unique in mFirstLetterIndex.
	std::sort(mFirstLetterIndex.begin(), mFirstLetterIndex.end());
	auto last = std::unique(mFirstLetterIndex.begin(), mFirstLetterIndex.end());
	mFirstLetterIndex.erase(last, mFirstLetterIndex.end());

	if (!ascending)
		std::reverse(mChildren.begin(), mChildren.end());
}

void FileData::sortFavoritesOnTop(ComparisonFunction& comparator, bool ascending)
{
	mFirstLetterIndex.clear();
	std::vector<FileData*> mChildrenFavorites;
	std::vector<FileData*> mChildrenOthers;

	for (unsigned int i = 0; i < mChildren.size(); i++) {
		if (mChildren[i]->getFavorite()) {
			mChildrenFavorites.push_back(mChildren[i]);
		}
		else {
			mChildrenOthers.push_back(mChildren[i]);
			// Build mFirstLetterIndex.
			const char firstChar = toupper(mChildren[i]->getSortName().front());
			mFirstLetterIndex.push_back(std::string(1, firstChar));
		}
	}

	// If there are only favorites in the gamelist, it makes sense to still generate
	// a letter index. For instance to be able to quick jump in the 'favorites'
	// collection. Doing this additional work here only for the applicable gamelists is
	// probably faster than building a redundant index for all gamelists during sorting.
	if (mChildrenOthers.size() == 0 && mChildrenFavorites.size() > 0) {
		for (unsigned int i = 0; i < mChildren.size(); i++) {
			const char firstChar = toupper(mChildren[i]->getSortName().front());
			mFirstLetterIndex.push_back(std::string(1, firstChar));
		}
	}

	// Sort and make each entry unique in mFirstLetterIndex.
	std::sort(mFirstLetterIndex.begin(), mFirstLetterIndex.end());
	auto last = std::unique(mFirstLetterIndex.begin(), mFirstLetterIndex.end());
	mFirstLetterIndex.erase(last, mFirstLetterIndex.end());

	// If there were at least one favorite in the gamelist, insert the favorite
	// unicode character in the first position.
	if (mChildrenOthers.size() > 0 && mChildrenFavorites.size() > 0)
		mFirstLetterIndex.insert(mFirstLetterIndex.begin(), FAVORITE_CHAR);

	// Sort favorite games and the other games separately.
	std::stable_sort(mChildrenFavorites.begin(), mChildrenFavorites.end(), comparator);
	std::stable_sort(mChildrenOthers.begin(), mChildrenOthers.end(), comparator);

	// Iterate through any child folders.
	for (auto it = mChildrenFavorites.cbegin(); it != mChildrenFavorites.cend(); it++) {
		if ((*it)->getChildren().size() > 0)
			(*it)->sortFavoritesOnTop(comparator, ascending);
	}

	// Iterate through any child folders.
	for (auto it = mChildrenOthers.cbegin(); it != mChildrenOthers.cend(); it++) {
		if ((*it)->getChildren().size() > 0)
			(*it)->sortFavoritesOnTop(comparator, ascending);
	}

	if (!ascending) {
		std::reverse(mChildrenFavorites.begin(), mChildrenFavorites.end());
		std::reverse(mChildrenOthers.begin(), mChildrenOthers.end());
	}

	// Combine the individually sorted favorite games and other games vectors.
	mChildren.erase(mChildren.begin(), mChildren.end());
	mChildren.reserve(mChildrenFavorites.size() + mChildrenOthers.size());
	mChildren.insert(mChildren.end(), mChildrenFavorites.begin(), mChildrenFavorites.end());
	mChildren.insert(mChildren.end(), mChildrenOthers.begin(), mChildrenOthers.end());
}

void FileData::sort(const SortType& type, bool mFavoritesOnTop)
{
	if (mFavoritesOnTop)
		sortFavoritesOnTop(*type.comparisonFunction, type.ascending);
	else
		sort(*type.comparisonFunction, type.ascending);
}

void FileData::launchGame(Window* window)
{
	LOG(LogInfo) << "Attempting to launch game...";

	AudioManager::getInstance()->deinit();
	VolumeControl::getInstance()->deinit();

//	window->deinit();

	std::string command = "";

	// Check if there is a launch string override for the game
	// and the corresponding option to use it has been set.
	if (Settings::getInstance()->getBool("LaunchstringOverride") &&
			!metadata.get("launchstring").empty())
		command = metadata.get("launchstring");
	else
		command = mEnvData->mLaunchCommand;

	const std::string rom      = Utils::FileSystem::getEscapedPath(getPath());
	const std::string basename = Utils::FileSystem::getStem(getPath());
	const std::string rom_raw  = Utils::FileSystem::getPreferredPath(getPath());

	command = Utils::String::replace(command, "%ROM%", rom);
	command = Utils::String::replace(command, "%BASENAME%", basename);
	command = Utils::String::replace(command, "%ROM_RAW%", rom_raw);

	Scripting::fireEvent("game-start", rom, basename);

	LOG(LogInfo) << "	" << command;
	int exitCode = runSystemCommand(command);

	if (exitCode != 0)
		LOG(LogWarning) << "...launch terminated with nonzero exit code " << exitCode << "!";

	Scripting::fireEvent("game-end");

//	window->init();

	VolumeControl::getInstance()->init();
	window->normalizeNextUpdate();

	// Update number of times the game has been launched.
	FileData* gameToUpdate = getSourceFileData();

	int timesPlayed = gameToUpdate->metadata.getInt("playcount") + 1;
	gameToUpdate->metadata.set("playcount", std::to_string(static_cast<long long>(timesPlayed)));

	// Update last played time.
	gameToUpdate->metadata.set("lastplayed", Utils::Time::DateTime(Utils::Time::now()));
	CollectionSystemManager::get()->refreshCollectionSystems(gameToUpdate);

	gameToUpdate->mSystem->onMetaDataSavePoint();
}

CollectionFileData::CollectionFileData(FileData* file, SystemData* system)
	: FileData(file->getSourceFileData()->getType(), file->getSourceFileData()->getPath(),
			file->getSourceFileData()->getSystemEnvData(), system)
{
	// We use this constructor to create a clone of the filedata, and change its system.
	mSourceFileData = file->getSourceFileData();
	refreshMetadata();
	mParent = NULL;
	metadata = mSourceFileData->metadata;
	mSystemName = mSourceFileData->getSystem()->getName();
}

CollectionFileData::~CollectionFileData()
{
	// Need to remove collection file data at the collection object destructor.
	if (mParent)
		mParent->removeChild(this);
	mParent = NULL;
}

std::string CollectionFileData::getKey() {
	return getFullPath();
}

FileData* CollectionFileData::getSourceFileData()
{
	return mSourceFileData;
}

void CollectionFileData::refreshMetadata()
{
	metadata = mSourceFileData->metadata;
	mDirty = true;
}

const std::string& CollectionFileData::getName()
{
	if (mDirty) {
		mCollectionFileName =
				Utils::String::removeParenthesis(mSourceFileData->metadata.get("name"));
		mCollectionFileName +=
				" [" + Utils::String::toUpper(mSourceFileData->getSystem()->getName()) + "]";
		mDirty = false;
	}

	if (Settings::getInstance()->getBool("CollectionShowSystemInfo"))
		return mCollectionFileName;

	return mSourceFileData->metadata.get("name");
}

// Return sort type based on a string description.
FileData::SortType getSortTypeFromString(std::string desc) {
	std::vector<FileData::SortType> SortTypes = FileSorts::SortTypes;
	// Find it
	for (unsigned int i = 0; i < FileSorts::SortTypes.size(); i++) {
		const FileData::SortType& sort = FileSorts::SortTypes.at(i);
		if (sort.description == desc)
			return sort;
	}
	// If no type found then default to "filename, ascending".
	return FileSorts::SortTypes.at(0);
}
