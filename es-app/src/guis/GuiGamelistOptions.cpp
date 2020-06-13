//
//	GuiGamelistOptions.cpp
//
//	Gamelist options menu for the 'Jump to...' quick selector,
//	game sorting, game filters, and metadata edit.
//
//	The filter interface is covered by GuiGamelistFilter and the
//	metadata edit interface is covered by GuiMetaDataEd.
//

#include "GuiGamelistOptions.h"

#include "guis/GuiGamelistFilter.h"
#include "scrapers/Scraper.h"
#include "views/gamelist/IGameListView.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "CollectionSystemManager.h"
#include "FileFilterIndex.h"
#include "FileSorts.h"
#include "GuiMetaDataEd.h"
#include "SystemData.h"
#include "Sound.h"

GuiGamelistOptions::GuiGamelistOptions(
			Window* window,
			SystemData* system)
			: GuiComponent(window),
			mSystem(system),
			mMenu(window, "OPTIONS"),
			fromPlaceholder(false),
			mFiltersChanged(false),
			mCancelled(false)
{
	addChild(&mMenu);

	// Check that it's not a placeholder folder - if it is, only show "Filter Options".
	FileData* file = getGamelist()->getCursor();
	fromPlaceholder = file->isPlaceHolder();
	ComponentListRow row;

	// Read the applicable favorite sorting setting depending on whether the
	// system is a custom collection or not.
	if (CollectionSystemManager::get()->getIsCustomCollection(file->getSystem()))
		mFavoritesSorting = Settings::getInstance()->getBool("FavFirstCustom");
	else
		mFavoritesSorting = Settings::getInstance()->getBool("FavoritesFirst");

	if (!fromPlaceholder) {
		// Jump to letter quick selector.
		row.elements.clear();

		// The letter index is generated in FileData during gamelist sorting.
		mFirstLetterIndex = file->getParent()->getFirstLetterIndex();

		// Set the quick selector to the first character of the selected game.
		if (mFavoritesSorting && file->getFavorite() && mFirstLetterIndex.front() == FAVORITE_CHAR)
			mCurrentFirstCharacter = FAVORITE_CHAR;
		else
			mCurrentFirstCharacter = toupper(file->getSortName().front());

		mJumpToLetterList = std::make_shared<LetterList>(mWindow, getHelpStyle(),
				 "JUMP TO...", false);

		// Populate the quick selector.
		for (unsigned int i = 0; i < mFirstLetterIndex.size(); i++) {
			mJumpToLetterList->add(mFirstLetterIndex[i], mFirstLetterIndex[i], 0);
			if (mFirstLetterIndex[i] == mCurrentFirstCharacter)
				mJumpToLetterList->selectEntry(i);
		}

		if (system->getName() != "recent")
			mMenu.addWithLabel("JUMP TO..", mJumpToLetterList);

		// Sort list by selected sort type (persistent throughout the program session).
		mListSort = std::make_shared<SortList>(mWindow, getHelpStyle(), "SORT GAMES BY", false);
		FileData* root = mSystem->getRootFolder();
		std::string sortType = root->getSortTypeString();

		for (unsigned int i = 0; i <FileSorts::SortTypes.size(); i++) {
			const FileData::SortType& sort = FileSorts::SortTypes.at(i);
			if (sort.description == sortType)
				mListSort->add(sort.description, &sort, 1);
			else
				mListSort->add(sort.description, &sort, 0);
		}
		// Don't show the sort type option if the gamelist type is recent/last played.
		if (system->getName() != "recent")
			mMenu.addWithLabel("SORT GAMES BY", mListSort);
	}

	// Show filtered menu.
	if (system->getName() != "recent" && !Settings::getInstance()->getBool("ForceDisableFilters")) {
		row.elements.clear();
		row.addElement(std::make_shared<TextComponent>
				(mWindow, "FILTER GAMELIST", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
		row.addElement(makeArrow(mWindow), false);
		row.makeAcceptInputHandler(std::bind(&GuiGamelistOptions::openGamelistFilter, this));
		mMenu.addRow(row);
	}

	std::map<std::string, CollectionSystemData> customCollections =
			CollectionSystemManager::get()->getCustomCollectionSystems();

	if (UIModeController::getInstance()->isUIModeFull() &&
		((customCollections.find(system->getName()) != customCollections.cend() &&
		CollectionSystemManager::get()->getEditingCollection() != system->getName()) ||
		CollectionSystemManager::get()->getCustomCollectionsBundle()->getName() ==
				system->getName())) {
		row.elements.clear();
		row.addElement(std::make_shared<TextComponent>(
				mWindow, "ADD/REMOVE GAMES TO THIS GAME COLLECTION", Font::get(FONT_SIZE_MEDIUM),
				0x777777FF), true);
		row.makeAcceptInputHandler(std::bind(&GuiGamelistOptions::startEditMode, this));
		mMenu.addRow(row);
	}

	if (UIModeController::getInstance()->isUIModeFull() &&
			CollectionSystemManager::get()->isEditing()) {
		row.elements.clear();
		row.addElement(std::make_shared<TextComponent>(
					mWindow, "FINISH EDITING '" + Utils::String::toUpper(
					CollectionSystemManager::get()->getEditingCollection()) +
					"' COLLECTION",Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
		row.makeAcceptInputHandler(std::bind(&GuiGamelistOptions::exitEditMode, this));
		mMenu.addRow(row);
	}

	if (UIModeController::getInstance()->isUIModeFull() && !fromPlaceholder &&
			!(mSystem->isCollection() && file->getType() == FOLDER)) {
		row.elements.clear();
		row.addElement(std::make_shared<TextComponent>(mWindow,
				"EDIT THIS GAME'S METADATA", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
		row.addElement(makeArrow(mWindow), false);
		row.makeAcceptInputHandler(std::bind(&GuiGamelistOptions::openMetaDataEd, this));
		mMenu.addRow(row);
	}

	// Buttons. Logic to apply or cancel settings are handled by the destructor.
	mMenu.addButton("APPLY", "apply", [&] { delete this; });
	mMenu.addButton("CANCEL", "cancel", [&] { mCancelled = true; delete this; });

	// Center the menu.
	setSize((float)Renderer::getScreenWidth(), (float)Renderer::getScreenHeight());
	mMenu.setPosition((mSize.x() - mMenu.getSize().x()) / 2, (mSize.y() -
			mMenu.getSize().y()) / 2);
}

GuiGamelistOptions::~GuiGamelistOptions()
{
	if (mCancelled)
		return;

	if (!fromPlaceholder) {
		FileData* root = mSystem->getRootFolder();

		// If a new sorting type was selected, then sort and update mSortTypeString for the system.
		if ((*mListSort->getSelected()).description != root->getSortTypeString()) {
			// This will also recursively sort children.
			root->sort(*mListSort->getSelected(), mFavoritesSorting);
			root->setSortTypeString((*mListSort->getSelected()).description);

			// Notify that the root folder was sorted (refresh).
			getGamelist()->onFileChanged(root, FILE_SORTED);
		}

		// Has the user changed the letter using the quick selector?
		if (mCurrentFirstCharacter != mJumpToLetterList->getSelected()) {
			if (mJumpToLetterList->getSelected() == FAVORITE_CHAR)
				jumpToFirstRow();
			else
				jumpToLetter();
		}
	}

	if (mFiltersChanged) {
		// Only reload full view if we came from a placeholder as we need to
		// re-display the remaining elements for whatever new game is selected.
		ViewController::get()->reloadGameListView(mSystem);
	}

	NavigationSounds::getInstance()->playThemeNavigationSound(SCROLLSOUND);
}

void GuiGamelistOptions::openGamelistFilter()
{
	mFiltersChanged = true;
	GuiGamelistFilter* ggf = new GuiGamelistFilter(mWindow, mSystem);
	mWindow->pushGui(ggf);
}

void GuiGamelistOptions::startEditMode()
{
	std::string editingSystem = mSystem->getName();
	// Need to check if we're editing the collections bundle,
	// as we will want to edit the selected collection within.
	if (editingSystem == CollectionSystemManager::get()->getCustomCollectionsBundle()->getName()) {
		FileData* file = getGamelist()->getCursor();
		// Do we have the cursor on a specific collection?.
		if (file->getType() == FOLDER)
			editingSystem = file->getName();
		else
			// We are inside a specific collection. We want to edit that one.
			editingSystem = file->getSystem()->getName();
	}
	CollectionSystemManager::get()->setEditMode(editingSystem);
	delete this;
}

void GuiGamelistOptions::exitEditMode()
{
	CollectionSystemManager::get()->exitEditMode();
	delete this;
}

void GuiGamelistOptions::openMetaDataEd()
{
	// Open metadata editor.
	// Get the FileData that holds the original metadata.
	FileData* file = getGamelist()->getCursor()->getSourceFileData();
	ScraperSearchParams p;
	p.game = file;
	p.system = file->getSystem();

	std::function<void()> deleteBtnFunc;

	if (file->getType() == FOLDER) {
		deleteBtnFunc = NULL;
	}
	else {
		deleteBtnFunc = [this, file] {
			CollectionSystemManager::get()->deleteCollectionFiles(file);
			ViewController::get()->getGameListView(file->getSystem()).get()->remove(file, true);
		};
	}

	mWindow->pushGui(new GuiMetaDataEd(mWindow, &file->metadata, file->metadata.getMDD(), p,
			Utils::FileSystem::getFileName(file->getPath()), std::bind(
			&IGameListView::onFileChanged, ViewController::get()->getGameListView(
			file->getSystem()).get(), file, FILE_METADATA_CHANGED), deleteBtnFunc));
}

void GuiGamelistOptions::jumpToLetter()
{
	char letter = mJumpToLetterList->getSelected().front();

	// Get the gamelist.
	const std::vector<FileData*>& files = getGamelist()->getCursor()->
			getParent()->getChildrenListToDisplay();

	for (unsigned int i = 0; i < files.size(); i++) {
		if (mFavoritesSorting && mFirstLetterIndex.front() == FAVORITE_CHAR) {
			if ((char)toupper(files.at(i)->getSortName().front()) ==
					letter && !files.at(i)->getFavorite()) {
				getGamelist()->setCursor(files.at(i));
				break;
			}
		}
		else {
			if ((char)toupper(files.at(i)->getSortName().front()) == letter) {
				getGamelist()->setCursor(files.at(i));
				break;
			}
		}
	}
}

void GuiGamelistOptions::jumpToFirstRow()
{
	// Get first row of the gamelist.
	getGamelist()->setCursor(getGamelist()->getFirstEntry());
}

bool GuiGamelistOptions::input(InputConfig* config, Input input)
{
	if (input.value != 0 && config->isMappedTo("select", input))
		mCancelled = true;

	if (input.value != 0 && (config->isMappedTo("b", input) ||
			config->isMappedTo("select", input))) {
		delete this;
		return true;
	}

	return mMenu.input(config, input);
}

HelpStyle GuiGamelistOptions::getHelpStyle()
{
	HelpStyle style = HelpStyle();
	style.applyTheme(mSystem->getTheme(), "system");
	return style;
}

std::vector<HelpPrompt> GuiGamelistOptions::getHelpPrompts()
{
	auto prompts = mMenu.getHelpPrompts();
	prompts.push_back(HelpPrompt("a", "select"));
	prompts.push_back(HelpPrompt("b", "close (apply)"));
	prompts.push_back(HelpPrompt("select", "close (cancel)"));
	return prompts;
}

IGameListView* GuiGamelistOptions::getGamelist()
{
	return ViewController::get()->getGameListView(mSystem).get();
}
