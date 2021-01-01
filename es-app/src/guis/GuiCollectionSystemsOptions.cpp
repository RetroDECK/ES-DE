//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiCollectionSystemsOptions.cpp
//
//  User interface for the game collection settings.
//  Submenu to the GuiMenu main menu.
//

#include "guis/GuiCollectionSystemsOptions.h"

#include "components/OptionListComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiSettings.h"
#include "guis/GuiTextEditPopup.h"
#include "views/ViewController.h"
#include "CollectionSystemsManager.h"

GuiCollectionSystemsOptions::GuiCollectionSystemsOptions(
        Window* window,
        std::string title)
        : GuiSettings(window, title),
        mAddedCustomCollection(false),
        mDeletedCustomCollection(false)
{
    // Finish editing custom collection.
    if (CollectionSystemsManager::get()->isEditing()) {
        ComponentListRow row;
        row.addElement(std::make_shared<TextComponent>(mWindow, "FINISH EDITING '" +
                Utils::String::toUpper(CollectionSystemsManager::get()->getEditingCollection()) +
                "' COLLECTION", Font::get(FONT_SIZE_MEDIUM), 0x777777FF), true);
        row.makeAcceptInputHandler([this] {
            CollectionSystemsManager::get()->exitEditMode();
            mWindow->invalidateCachedBackground();
            delete this;
        });
        addRow(row);
    }

    // Automatic collections.
    collection_systems_auto = std::make_shared<OptionListComponent<std::string>>
            (mWindow, getHelpStyle(), "SELECT COLLECTIONS", true);
    std::map<std::string, CollectionSystemData, stringComparator> autoSystems =
            CollectionSystemsManager::get()->getAutoCollectionSystems();
    // Add automatic systems.
    for (std::map<std::string, CollectionSystemData, stringComparator>::const_iterator
            it = autoSystems.cbegin(); it != autoSystems.cend() ; it++)
        collection_systems_auto->add(it->second.decl.longName, it->second.decl.name,
                it->second.isEnabled);
    addWithLabel("AUTOMATIC GAME COLLECTIONS", collection_systems_auto);
    addSaveFunc([this] {
        std::string autoSystemsSelected =
            Utils::String::vectorToCommaString(collection_systems_auto->getSelectedObjects(), true);
        std::string autoSystemsConfig = Settings::getInstance()->getString("CollectionSystemsAuto");
        if (autoSystemsSelected != autoSystemsConfig) {
            if (CollectionSystemsManager::get()->isEditing())
                CollectionSystemsManager::get()->exitEditMode();
            Settings::getInstance()->setString("CollectionSystemsAuto", autoSystemsSelected);
            setNeedsSaving();
            setNeedsReloading();
            setNeedsCollectionsUpdate();
            if (!mAddedCustomCollection)
                setNeedsGoToSystemView(SystemData::sSystemVector.front());
        }
    });

    // Custom collections.
    collection_systems_custom = std::make_shared<OptionListComponent<std::string>>
            (mWindow, getHelpStyle(), "SELECT COLLECTIONS", true);
    std::map<std::string, CollectionSystemData, stringComparator> customSystems =
            CollectionSystemsManager::get()->getCustomCollectionSystems();
    // Add custom systems.
    for (std::map<std::string, CollectionSystemData, stringComparator>::const_iterator
            it = customSystems.cbegin(); it != customSystems.cend() ; it++)
        collection_systems_custom->add(it->second.decl.longName, it->second.decl.name,
                it->second.isEnabled);
    addWithLabel("CUSTOM GAME COLLECTIONS", collection_systems_custom);
    addSaveFunc([this] {
        if (!mDeletedCustomCollection) {
            std::string customSystemsSelected = Utils::String::vectorToCommaString(
                    collection_systems_custom->getSelectedObjects(), true);
            std::string customSystemsConfig = Settings::getInstance()->
                    getString("CollectionSystemsCustom");
            if (customSystemsSelected != customSystemsConfig) {
                if (CollectionSystemsManager::get()->isEditing())
                    CollectionSystemsManager::get()->exitEditMode();
                Settings::getInstance()->setString("CollectionSystemsCustom",
                        customSystemsSelected);
                setNeedsSaving();
                setNeedsReloading();
                setNeedsCollectionsUpdate();
                if (!mAddedCustomCollection)
                    setNeedsGoToSystemView(SystemData::sSystemVector.front());
            }
        }
    });

    // Create custom collection from theme.
    std::vector<std::string> unusedFolders =
            CollectionSystemsManager::get()->getUnusedSystemsFromTheme();
    if (unusedFolders.size() > 0) {
        ComponentListRow row;
        auto themeCollection = std::make_shared<TextComponent>(mWindow,
                "CREATE NEW CUSTOM COLLECTION FROM THEME", Font::get(FONT_SIZE_MEDIUM), 0x777777FF);
        auto bracketThemeCollection = std::make_shared<ImageComponent>(mWindow);
        bracketThemeCollection->setImage(":/graphics/arrow.svg");
        bracketThemeCollection->setResize(Vector2f(0,
                Font::get(FONT_SIZE_MEDIUM)->getLetterHeight()));
        row.addElement(themeCollection, true);
        row.addElement(bracketThemeCollection, false);
        row.makeAcceptInputHandler([this, unusedFolders] {
            auto ss = new GuiSettings(mWindow, "SELECT THEME FOLDER");
            std::shared_ptr<OptionListComponent<std::string>> folderThemes =
                    std::make_shared<OptionListComponent<std::string>>(mWindow,
                    getHelpStyle(), "SELECT THEME FOLDER", true);
            // Add custom systems.
            for (auto it = unusedFolders.cbegin() ; it != unusedFolders.cend() ; it++ ) {
                ComponentListRow row;
                std::string name = *it;
                std::function<void()> createCollectionCall = [this, name] {
                    createCustomCollection(name);
                };
                row.makeAcceptInputHandler(createCollectionCall);
                auto themeFolder = std::make_shared<TextComponent>(mWindow,
                        Utils::String::toUpper(name), Font::get(FONT_SIZE_SMALL), 0x777777FF);
                row.addElement(themeFolder, true);
                // This transparent bracket is only added to generate the correct help prompts.
                auto bracket = std::make_shared<ImageComponent>(mWindow);
                bracket->setImage(":/graphics/arrow.svg");
                bracket->setOpacity(0);
                row.addElement(bracket, false);
                ss->addRow(row);
            }
            mWindow->pushGui(ss);
        });
        addRow(row);
    }

    // Create new custom collection.
    ComponentListRow row;
    auto newCollection = std::make_shared<TextComponent>(mWindow,
            "CREATE NEW CUSTOM COLLECTION", Font::get(FONT_SIZE_MEDIUM), 0x777777FF);
    auto bracketNewCollection = std::make_shared<ImageComponent>(mWindow);
    bracketNewCollection->setImage(":/graphics/arrow.svg");
    bracketNewCollection->setResize(Vector2f(0,
            Font::get(FONT_SIZE_MEDIUM)->getLetterHeight()));
    row.addElement(newCollection, true);
    row.addElement(bracketNewCollection, false);
    auto createCollectionCall = [this](const std::string& newVal) {
        std::string name = newVal;
        // We need to store the first GUI and remove it, as it'll be deleted
        // by the actual GUI.
        Window* window = mWindow;
        GuiComponent* topGui = window->peekGui();
        window->removeGui(topGui);
        createCustomCollection(name);
    };
    row.makeAcceptInputHandler([this, createCollectionCall] {
        mWindow->pushGui(new GuiTextEditPopup(mWindow, getHelpStyle(),
                "New Collection Name", "", createCollectionCall, false, "SAVE"));
    });
    addRow(row);

    // Delete custom collection.
    row.elements.clear();
    auto deleteCollection = std::make_shared<TextComponent>(mWindow,
            "DELETE CUSTOM COLLECTION", Font::get(FONT_SIZE_MEDIUM), 0x777777FF);
    auto bracketDeleteCollection = std::make_shared<ImageComponent>(mWindow);
    bracketDeleteCollection->setImage(":/graphics/arrow.svg");
    bracketDeleteCollection->setResize(Vector2f(0,
            Font::get(FONT_SIZE_MEDIUM)->getLetterHeight()));
    row.addElement(deleteCollection, true);
    row.addElement(bracketDeleteCollection, false);
    row.makeAcceptInputHandler([this, customSystems] {
        auto ss = new GuiSettings(mWindow, "SELECT COLLECTION TO DELETE");
        std::shared_ptr<OptionListComponent<std::string>> customCollections =
                std::make_shared<OptionListComponent<std::string>>(mWindow,
                getHelpStyle(), "", true);
        for (std::map<std::string, CollectionSystemData, stringComparator>::const_iterator
                it = customSystems.cbegin(); it != customSystems.cend() ; it++) {
            ComponentListRow row;
            std::string name = (*it).first;
            std::function<void()> deleteCollectionCall = [this, name] {
                mWindow->pushGui(new GuiMsgBox(mWindow, getHelpStyle(),
                        "THIS WILL PERMANENTLY\nDELETE THE COLLECTION\n'" +
                        Utils::String::toUpper(name) + "'\n"
                        "ARE YOU SURE?",
                        "YES", [this, name] {
                            if (CollectionSystemsManager::get()->isEditing())
                                CollectionSystemsManager::get()->exitEditMode();
                            mDeletedCustomCollection = true;
                            std::vector<std::string> selectedCustomCollections =
                                    collection_systems_custom->getSelectedObjects();
                            std::string collectionsConfigEntry;
                            // Create the configuration file entry. If the collection to be
                            // deleted was activated, then exclude it.
                            for (auto it = selectedCustomCollections.begin();
                                    it != selectedCustomCollections.end(); it++) {
                                if ((*it) != name) {
                                    if ((*it) != selectedCustomCollections.front() &&
                                            collectionsConfigEntry != "")
                                        collectionsConfigEntry += ",";
                                    collectionsConfigEntry += (*it);
                                }
                            }
                            // If the system to be deleted was present in es_settings.cfg, we
                            // need to re-write it.
                            if (collectionsConfigEntry !=
                                    Settings::getInstance()->getString("CollectionSystemsCustom")) {
                                Settings::getInstance()->setString("CollectionSystemsCustom",
                                        collectionsConfigEntry);
                                setNeedsSaving();
                                setNeedsGoToSystemView(SystemData::sSystemVector.front());
                            }
                            CollectionSystemsManager::get()->deleteCustomCollection(name);
                            return true;
                        },
                        "NO", [this] {
                            return false;
                        }));
            };
            row.makeAcceptInputHandler(deleteCollectionCall);
            auto customCollection = std::make_shared<TextComponent>(mWindow,
                    Utils::String::toUpper(name), Font::get(FONT_SIZE_SMALL), 0x777777FF);
            row.addElement(customCollection, true);
            // This transparent bracket is only added generate the correct help prompts.
            auto bracket = std::make_shared<ImageComponent>(mWindow);
            bracket->setImage(":/graphics/arrow.svg");
            bracket->setOpacity(0);
            row.addElement(bracket, false);
            ss->addRow(row);
        }
        mWindow->pushGui(ss);
    });
    addRow(row);

    // Sort favorites on top for custom collections.
    auto fav_first_custom = std::make_shared<SwitchComponent>(mWindow);
    fav_first_custom->setState(Settings::getInstance()->getBool("FavFirstCustom"));
    addWithLabel("SORT FAVORITES ON TOP FOR CUSTOM COLLECTIONS", fav_first_custom);
    addSaveFunc([this, fav_first_custom] {
        if (fav_first_custom->getState() != Settings::getInstance()->getBool("FavFirstCustom")) {
            Settings::getInstance()->setBool("FavFirstCustom", fav_first_custom->getState());
            setNeedsSaving();
            setNeedsReloading();
            setNeedsSorting();
            setNeedsSortingCollections();
        }
    });

    // Display star markings for custom collections.
    auto fav_star_custom = std::make_shared<SwitchComponent>(mWindow);
    fav_star_custom->setState(Settings::getInstance()->getBool("FavStarCustom"));
    addWithLabel("DISPLAY STAR MARKINGS FOR CUSTOM COLLECTIONS", fav_star_custom);
    addSaveFunc([this, fav_star_custom] {
        if (fav_star_custom->getState() != Settings::getInstance()->getBool("FavStarCustom")) {
            Settings::getInstance()->setBool("FavStarCustom", fav_star_custom->getState());
            setNeedsSaving();
            setNeedsReloading();
        }
    });

    // Group unthemed custom collections.
    auto use_custom_collections_system = std::make_shared<SwitchComponent>(mWindow);
    use_custom_collections_system->setState(Settings::getInstance()->
            getBool("UseCustomCollectionsSystem"));
    addWithLabel("GROUP UNTHEMED CUSTOM COLLECTIONS", use_custom_collections_system);
    addSaveFunc([this, use_custom_collections_system] {
        if (use_custom_collections_system->getState() !=
                Settings::getInstance()->getBool("UseCustomCollectionsSystem")) {
            Settings::getInstance()->setBool("UseCustomCollectionsSystem",
                    use_custom_collections_system->getState());
            setNeedsSaving();
            setNeedsCollectionsUpdate();
            setNeedsReloading();
            setNeedsGoToSystemView(SystemData::sSystemVector.front());
        }
    });

    // Show system names in collections.
    auto collection_show_system_info = std::make_shared<SwitchComponent>(mWindow);
    collection_show_system_info->setState(Settings::getInstance()->
            getBool("CollectionShowSystemInfo"));
    addWithLabel("SHOW SYSTEM NAMES IN COLLECTIONS", collection_show_system_info);
    addSaveFunc([this, collection_show_system_info] {
        if (collection_show_system_info->getState() !=
                Settings::getInstance()->getBool("CollectionShowSystemInfo")) {
            Settings::getInstance()->setBool("CollectionShowSystemInfo",
                    collection_show_system_info->getState());
            setNeedsSaving();
            setNeedsReloading();
        }
    });
}

void GuiCollectionSystemsOptions::createCustomCollection(std::string inName)
{
    if (CollectionSystemsManager::get()->isEditing())
        CollectionSystemsManager::get()->exitEditMode();

    std::string collectionName = CollectionSystemsManager::get()->
            getValidNewCollectionName(inName);
    SystemData* newCollection = CollectionSystemsManager::get()->
            addNewCustomCollection(collectionName);

    CollectionSystemsManager::get()->saveCustomCollection(newCollection);
    collection_systems_custom->add(collectionName, collectionName, true);

    mAddedCustomCollection = true;
    setNeedsGoToSystemView(newCollection);

    Window* window = mWindow;
    while (window->peekGui() && window->peekGui() != ViewController::get())
        delete window->peekGui();
    CollectionSystemsManager::get()->setEditMode(collectionName);
}
