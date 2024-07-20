//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiCollectionSystemsOptions.cpp
//
//  User interface for the game collection settings.
//  Submenu to the GuiMenu main menu.
//

#include "guis/GuiCollectionSystemsOptions.h"

#include "CollectionSystemsManager.h"
#include "components/OptionListComponent.h"
#include "components/SwitchComponent.h"
#include "guis/GuiMsgBox.h"
#include "guis/GuiSettings.h"
#include "guis/GuiTextEditKeyboardPopup.h"
#include "guis/GuiTextEditPopup.h"
#include "utils/LocalizationUtil.h"
#include "utils/StringUtil.h"
#include "views/ViewController.h"

GuiCollectionSystemsOptions::GuiCollectionSystemsOptions(std::string title)
    : GuiSettings {title}
    , mRenderer {Renderer::getInstance()}
    , mAddedCustomCollection {false}
    , mDeletedCustomCollection {false}
{
    // Finish editing custom collection.
    if (CollectionSystemsManager::getInstance()->isEditing()) {
        ComponentListRow row;
        const std::string editingText {Utils::String::format(
            _("FINISH EDITING '%s' COLLECTION"),
            Utils::String::toUpper(CollectionSystemsManager::getInstance()->getEditingCollection())
                .c_str())};
        row.addElement(std::make_shared<TextComponent>(editingText, Font::get(FONT_SIZE_MEDIUM),
                                                       mMenuColorPrimary),
                       true);
        row.makeAcceptInputHandler([this] {
            CollectionSystemsManager::getInstance()->exitEditMode();
            mWindow->invalidateCachedBackground();
            delete this;
        });
        addRow(row);
    }

    // Automatic collections.
    mCollectionSystemsAuto = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), _("SELECT COLLECTIONS"), true);
    std::map<std::string, CollectionSystemData, StringComparator> autoSystems {
        CollectionSystemsManager::getInstance()->getAutoCollectionSystems()};
    // Add automatic systems.
    for (std::map<std::string, CollectionSystemData, StringComparator>::const_iterator it =
             autoSystems.cbegin();
         it != autoSystems.cend(); ++it)
        mCollectionSystemsAuto->add(Utils::String::toUpper(_(it->second.decl.fullName.c_str())),
                                    it->second.decl.name, it->second.isEnabled);
    addWithLabel(_("AUTOMATIC GAME COLLECTIONS"), mCollectionSystemsAuto);
    addSaveFunc([this, autoSystems] {
        std::string autoSystemsSelected {Utils::String::vectorToDelimitedString(
            mCollectionSystemsAuto->getSelectedObjects(), ",", true)};
        std::string autoSystemsConfig {Settings::getInstance()->getString("CollectionSystemsAuto")};
        if (autoSystemsSelected != autoSystemsConfig) {
            if (CollectionSystemsManager::getInstance()->isEditing())
                CollectionSystemsManager::getInstance()->exitEditMode();
            Settings::getInstance()->setString("CollectionSystemsAuto", autoSystemsSelected);
            // Check if any systems have been enabled, and if so repopulate them, which results in
            // a complete initialization of their content. This is necessary as collections aren't
            // updated while they are disabled.
            std::vector<std::string> addedAutoSystems;
            if (autoSystemsConfig == "") {
                addedAutoSystems = Utils::String::delimitedStringToVector(autoSystemsSelected, ",");
            }
            else if (autoSystemsSelected != "") {
                std::vector<std::string> selectedVector {
                    Utils::String::delimitedStringToVector(autoSystemsSelected, ",")};
                std::vector<std::string> configuredVector {
                    Utils::String::delimitedStringToVector(autoSystemsConfig, ",")};
                for (std::string system : selectedVector) {
                    if (std::find(configuredVector.begin(), configuredVector.end(), system) ==
                        configuredVector.end())
                        addedAutoSystems.push_back(system);
                }
            }
            if (!addedAutoSystems.empty()) {
                for (std::string system : addedAutoSystems)
                    CollectionSystemsManager::getInstance()->repopulateCollection(
                        autoSystems.find(system)->second.system);
            }
            setNeedsSaving();
            setNeedsReloading();
            setNeedsCollectionsUpdate();
            setInvalidateCachedBackground();
        }
    });

    // Custom collections.
    mCollectionSystemsCustom = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), _("SELECT COLLECTIONS"), true);
    std::map<std::string, CollectionSystemData, StringComparator> customSystems {
        CollectionSystemsManager::getInstance()->getCustomCollectionSystems()};
    // Add custom systems.
    for (std::map<std::string, CollectionSystemData, StringComparator>::const_iterator it =
             customSystems.cbegin();
         it != customSystems.cend(); ++it)
        mCollectionSystemsCustom->add(Utils::String::toUpper(it->second.decl.fullName),
                                      it->second.decl.name, it->second.isEnabled);

    addWithLabel(_("CUSTOM GAME COLLECTIONS"), mCollectionSystemsCustom);
    addSaveFunc([this, customSystems] {
        if (!mDeletedCustomCollection) {
            std::string customSystemsSelected {Utils::String::vectorToDelimitedString(
                mCollectionSystemsCustom->getSelectedObjects(), ",", true)};
            std::string customSystemsConfig {
                Settings::getInstance()->getString("CollectionSystemsCustom")};
            if (customSystemsSelected != customSystemsConfig) {
                if (CollectionSystemsManager::getInstance()->isEditing())
                    CollectionSystemsManager::getInstance()->exitEditMode();
                Settings::getInstance()->setString("CollectionSystemsCustom",
                                                   customSystemsSelected);
                // Check if any systems have been enabled, and if so repopulate them, which
                // results in a complete initialization of their content. This is necessary as
                // collections aren't updated while they are disabled.
                std::vector<std::string> addedCustomSystems;
                if (customSystemsConfig == "") {
                    addedCustomSystems =
                        Utils::String::delimitedStringToVector(customSystemsSelected, ",");
                }
                else if (customSystemsSelected != "") {
                    std::vector<std::string> selectedVector {
                        Utils::String::delimitedStringToVector(customSystemsSelected, ",")};
                    std::vector<std::string> configuredVector {
                        Utils::String::delimitedStringToVector(customSystemsConfig, ",")};
                    for (std::string system : selectedVector) {
                        if (std::find(configuredVector.begin(), configuredVector.end(), system) ==
                            configuredVector.end())
                            addedCustomSystems.push_back(system);
                    }
                }
                if (!mAddedCustomCollection && !addedCustomSystems.empty()) {
                    for (std::string system : addedCustomSystems)
                        CollectionSystemsManager::getInstance()->repopulateCollection(
                            customSystems.find(system)->second.system);
                }
                setNeedsSaving();
                setNeedsReloading();
                setNeedsCollectionsUpdate();
                setNeedsGoToGroupedCollections();
                setInvalidateCachedBackground();
            }
        }
    });

    // If there are no custom collections, then gray out this menu entry.
    if (customSystems.empty()) {
        mCollectionSystemsCustom->setEnabled(false);
        mCollectionSystemsCustom->setOpacity(DISABLED_OPACITY);
        mCollectionSystemsCustom->getParent()
            ->getChild(mCollectionSystemsCustom->getChildIndex() - 1)
            ->setOpacity(DISABLED_OPACITY);
    }

    // Create custom collection from theme.
    std::vector<std::string> unusedFolders {
        CollectionSystemsManager::getInstance()->getUnusedSystemsFromTheme()};
    if (unusedFolders.size() > 0) {
        ComponentListRow row;
        auto themeCollection =
            std::make_shared<TextComponent>(_("CREATE NEW CUSTOM COLLECTION FROM THEME"),
                                            Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary);
        auto bracketThemeCollection = std::make_shared<ImageComponent>();
        bracketThemeCollection->setResize(
            glm::vec2 {0.0f, Font::get(FONT_SIZE_MEDIUM)->getLetterHeight()});
        bracketThemeCollection->setImage(":/graphics/arrow.svg");
        bracketThemeCollection->setColorShift(mMenuColorPrimary);
        row.addElement(themeCollection, true);
        row.addElement(bracketThemeCollection, false);
        row.makeAcceptInputHandler([this, unusedFolders] {
            auto ss = new GuiSettings(_("SELECT THEME FOLDER"));
            std::shared_ptr<OptionListComponent<std::string>> folderThemes {
                std::make_shared<OptionListComponent<std::string>>(getHelpStyle(),
                                                                   _("SELECT THEME FOLDER"), true)};
            // Add custom systems.
            for (auto it = unusedFolders.cbegin(); it != unusedFolders.cend(); ++it) {
                ComponentListRow row;
                std::string name {*it};
                std::function<void()> createCollectionCall {
                    [this, name] { createCustomCollection(name); }};
                row.makeAcceptInputHandler(createCollectionCall);
                auto themeFolder = std::make_shared<TextComponent>(
                    Utils::String::toUpper(name), Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary);
                themeFolder->setSelectable(true);
                row.addElement(themeFolder, true);
                ss->addRow(row);
            }
            mWindow->pushGui(ss);
        });
        addRow(row);
    }

    // Create new custom collection.
    ComponentListRow row;
    auto newCollection = std::make_shared<TextComponent>(
        _("CREATE NEW CUSTOM COLLECTION"), Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary);
    auto bracketNewCollection = std::make_shared<ImageComponent>();
    bracketNewCollection->setResize(
        glm::vec2 {0.0f, Font::get(FONT_SIZE_MEDIUM)->getLetterHeight()});
    bracketNewCollection->setImage(":/graphics/arrow.svg");
    bracketNewCollection->setColorShift(mMenuColorPrimary);
    row.addElement(newCollection, true);
    row.addElement(bracketNewCollection, false);
    auto createCollectionCall = [this](const std::string& newVal) {
        std::string name {newVal};
        // We need to store the first GUI and remove it, as it'll be deleted
        // by the actual GUI.
        Window* window {mWindow};
        GuiComponent* topGui {window->peekGui()};
        window->removeGui(topGui);
        createCustomCollection(name);
    };

    if (Settings::getInstance()->getBool("VirtualKeyboard")) {
        row.makeAcceptInputHandler([this, createCollectionCall] {
            const float verticalPosition {
                mRenderer->getIsVerticalOrientation() ? getMenu().getPosition().y : 0.0f};
            mWindow->pushGui(new GuiTextEditKeyboardPopup(
                getHelpStyle(), verticalPosition, _("NEW COLLECTION NAME"), "",
                createCollectionCall, false, _("CREATE"), _("CREATE COLLECTION?")));
        });
    }
    else {
        row.makeAcceptInputHandler([this, createCollectionCall] {
            mWindow->pushGui(new GuiTextEditPopup(getHelpStyle(), _("NEW COLLECTION NAME"), "",
                                                  createCollectionCall, false, _("CREATE"),
                                                  _("CREATE COLLECTION?")));
        });
    }
    addRow(row);

    // Delete custom collection.
    row.elements.clear();
    auto deleteCollection = std::make_shared<TextComponent>(
        _("DELETE CUSTOM COLLECTION"), Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary);
    auto bracketDeleteCollection = std::make_shared<ImageComponent>();
    bracketDeleteCollection->setResize(
        glm::vec2 {0.0f, Font::get(FONT_SIZE_MEDIUM)->getLetterHeight()});
    bracketDeleteCollection->setImage(":/graphics/arrow.svg");
    bracketDeleteCollection->setColorShift(mMenuColorPrimary);
    row.addElement(deleteCollection, true);
    row.addElement(bracketDeleteCollection, false);
    row.makeAcceptInputHandler([this, customSystems] {
        auto ss = new GuiSettings(_("COLLECTION TO DELETE"));
        std::shared_ptr<OptionListComponent<std::string>> customCollections {
            std::make_shared<OptionListComponent<std::string>>(getHelpStyle(), "", true)};
        for (std::map<std::string, CollectionSystemData, StringComparator>::const_iterator it =
                 customSystems.cbegin();
             it != customSystems.cend(); ++it) {
            ComponentListRow row;
            std::string name {(*it).first};
            std::function<void()> deleteCollectionCall = [this, name] {
                mWindow->pushGui(new GuiMsgBox(
                    getHelpStyle(),
                    Utils::String::format(
                        _("THIS WILL PERMANENTLY DELETE THE COLLECTION\n'%s'\nARE YOU SURE?"),
                        Utils::String::toUpper(name).c_str()),
                    _("YES"),
                    [this, name] {
                        if (CollectionSystemsManager::getInstance()->isEditing())
                            CollectionSystemsManager::getInstance()->exitEditMode();
                        mDeletedCustomCollection = true;
                        std::vector<std::string> selectedCustomCollections {
                            mCollectionSystemsCustom->getSelectedObjects()};
                        std::string collectionsConfigEntry;
                        // Create the configuration file entry. If the collection to be
                        // deleted was activated, then exclude it.
                        for (auto it = selectedCustomCollections.begin();
                             it != selectedCustomCollections.end(); ++it) {
                            if ((*it) != name) {
                                if ((*it) != selectedCustomCollections.front() &&
                                    collectionsConfigEntry != "")
                                    collectionsConfigEntry += ",";
                                collectionsConfigEntry += (*it);
                            }
                        }
                        // If the system to be deleted was present in es_settings.xml, we
                        // need to re-write it.
                        if (collectionsConfigEntry !=
                            Settings::getInstance()->getString("CollectionSystemsCustom")) {
                            Settings::getInstance()->setString("CollectionSystemsCustom",
                                                               collectionsConfigEntry);
                            if (selectedCustomCollections.size() == 1 &&
                                Settings::getInstance()->getString("StartupSystem") ==
                                    "collections")
                                Settings::getInstance()->setString("StartupSystem", "");
                            setNeedsSaving();
                            setNeedsGoToStart();
                        }
                        CollectionSystemsManager::getInstance()->deleteCustomCollection(name);
                        return true;
                    },
                    _("NO"), [] { return false; }, "", nullptr, nullptr, false, true,
                    (mRenderer->getIsVerticalOrientation() ?
                         0.43f :
                         0.28f * (1.778f / mRenderer->getScreenAspectRatio()))));
            };
            row.makeAcceptInputHandler(deleteCollectionCall);
            auto customCollection = std::make_shared<TextComponent>(
                Utils::String::toUpper(name), Font::get(FONT_SIZE_MEDIUM), mMenuColorPrimary);
            customCollection->setSelectable(true);
            row.addElement(customCollection, true);
            ss->addRow(row);
        }
        glm::vec3 menuPos {ss->getMenuPosition()};
        menuPos.x = (mRenderer->getScreenWidth() - ss->getMenuSize().x) / 2.0f;
        ss->setMenuPosition(menuPos);
        mWindow->pushGui(ss);
    });
    addRow(row);

    // Custom collections grouping.
    auto collectionCustomGrouping = std::make_shared<OptionListComponent<std::string>>(
        getHelpStyle(), _("GROUP CUSTOM COLLECTIONS"), false);
    const std::string& selectedCustomGrouping {
        Settings::getInstance()->getString("CollectionCustomGrouping")};
    collectionCustomGrouping->add(_("IF UNTHEMED"), "unthemed",
                                  selectedCustomGrouping == "unthemed");
    collectionCustomGrouping->add(_("ALWAYS"), "always", selectedCustomGrouping == "always");
    collectionCustomGrouping->add(_("NEVER"), "never", selectedCustomGrouping == "never");
    // If there are no objects returned, then there must be a manually modified entry in the
    // configuration file. Simply set custom collections grouping to "unthemed" in this case.
    if (collectionCustomGrouping->getSelectedObjects().size() == 0)
        collectionCustomGrouping->selectEntry(0);
    addWithLabel(_("GROUP CUSTOM COLLECTIONS"), collectionCustomGrouping);
    addSaveFunc([this, collectionCustomGrouping] {
        if (collectionCustomGrouping->getSelected() !=
            Settings::getInstance()->getString("CollectionCustomGrouping")) {
            Settings::getInstance()->setString("CollectionCustomGrouping",
                                               collectionCustomGrouping->getSelected());
            if (CollectionSystemsManager::getInstance()->isEditing())
                CollectionSystemsManager::getInstance()->exitEditMode();
            setNeedsSaving();
            setNeedsSorting();
            setNeedsSortingCollections();
            setNeedsCollectionsUpdate();
            setNeedsReloading();
            setNeedsGoToSystem(SystemData::sSystemVector.front());
            setInvalidateCachedBackground();
        }
    });

    // Sort favorites on top for custom collections.
    auto fav_first_custom = std::make_shared<SwitchComponent>();
    fav_first_custom->setState(Settings::getInstance()->getBool("FavFirstCustom"));
    addWithLabel(_("SORT FAVORITES ON TOP FOR CUSTOM COLLECTIONS"), fav_first_custom);
    addSaveFunc([this, fav_first_custom] {
        if (fav_first_custom->getState() != Settings::getInstance()->getBool("FavFirstCustom")) {
            Settings::getInstance()->setBool("FavFirstCustom", fav_first_custom->getState());
            setNeedsSaving();
            setNeedsReloading();
            setNeedsSorting();
            setNeedsSortingCollections();
            setInvalidateCachedBackground();
        }
    });

    // Display star markings for custom collections.
    auto fav_star_custom = std::make_shared<SwitchComponent>();
    fav_star_custom->setState(Settings::getInstance()->getBool("FavStarCustom"));
    addWithLabel(_("DISPLAY STAR MARKINGS FOR CUSTOM COLLECTIONS"), fav_star_custom);
    addSaveFunc([this, fav_star_custom] {
        if (fav_star_custom->getState() != Settings::getInstance()->getBool("FavStarCustom")) {
            Settings::getInstance()->setBool("FavStarCustom", fav_star_custom->getState());
            setNeedsSaving();
            setNeedsReloading();
            setInvalidateCachedBackground();
        }
    });
}

void GuiCollectionSystemsOptions::createCustomCollection(std::string inName)
{
    if (CollectionSystemsManager::getInstance()->isEditing())
        CollectionSystemsManager::getInstance()->exitEditMode();

    std::string collectionName {
        CollectionSystemsManager::getInstance()->getValidNewCollectionName(inName)};
    SystemData* newCollection {
        CollectionSystemsManager::getInstance()->addNewCustomCollection(collectionName)};

    CollectionSystemsManager::getInstance()->saveCustomCollection(newCollection);
    mCollectionSystemsCustom->add(collectionName, collectionName, true);

    mAddedCustomCollection = true;
    setNeedsGoToStart();

    if (Settings::getInstance()->getString("CollectionCustomGrouping") == "unthemed") {
        // We set both these flags as we don't know yet whether a theme exists for the collection.
        // It will be checked for properly in GuiSettings.
        setNeedsGoToGroupedCollections();
        setNeedsGoToSystem(newCollection);
    }
    else if (Settings::getInstance()->getString("CollectionCustomGrouping") == "always") {
        setNeedsGoToGroupedCollections();
    }
    else {
        setNeedsGoToSystem(newCollection);
    }

    LOG(LogInfo) << "Created new custom collection \"" << newCollection->getName() << "\"";

    Window* window {mWindow};
    while (window->peekGui() && window->peekGui() != ViewController::getInstance())
        delete window->peekGui();
    CollectionSystemsManager::getInstance()->setEditMode(collectionName);
}
