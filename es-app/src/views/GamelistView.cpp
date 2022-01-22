//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GamelistView.cpp
//
//  Main gamelist logic.
//

#include "views/GamelistView.h"
#include "views/GamelistLegacy.h"

#include "CollectionSystemsManager.h"
#include "UIModeController.h"
#include "animations/LambdaAnimation.h"

// #define FADE_IN_START_OPACITY 0.5f
// #define FADE_IN_TIME 325

GamelistView::GamelistView(FileData* root, bool legacyMode)
    : GamelistBase {root}
    , mLegacyMode {legacyMode}
    , mViewStyle {ViewController::BASIC}
{
    // TEMPORARY
    mLegacyMode = true;

    mViewStyle = ViewController::getInstance()->getState().viewstyle;

    if (mLegacyMode)
        return;

    const float padding {0.01f};

    mList.setPosition(mSize.x * (0.50f + padding), mList.getPosition().y);
    mList.setSize(mSize.x * (0.50f - padding), mList.getSize().y);
    mList.setAlignment(TextListComponent<FileData*>::ALIGN_LEFT);
    mList.setCursorChangedCallback([&](const CursorState& /*state*/) { updateInfoPanel(); });
}

GamelistView::~GamelistView()
{
    // Remove theme extras.
    for (auto extra : mThemeExtras) {
        removeChild(extra);
        delete extra;
    }
    mThemeExtras.clear();
}

void GamelistView::onFileChanged(FileData* file, bool reloadGamelist)
{
    if (reloadGamelist) {
        // Might switch to a detailed view.
        ViewController::getInstance()->reloadGamelistView(this);
        return;
    }

    // We could be tricky here to be efficient;
    // but this shouldn't happen very often so we'll just always repopulate.
    FileData* cursor {getCursor()};
    if (!cursor->isPlaceHolder()) {
        populateList(cursor->getParent()->getChildrenListToDisplay(), cursor->getParent());
        setCursor(cursor);
    }
    else {
        populateList(mRoot->getChildrenListToDisplay(), mRoot);
        setCursor(cursor);
    }
}

void GamelistView::onShow()
{
    // Reset any Lottie animations.
    for (auto extra : mThemeExtras)
        extra->resetFileAnimation();

    mLastUpdated = nullptr;
    GuiComponent::onShow();
    if (mLegacyMode)
        legacyUpdateInfoPanel();
    else
        updateInfoPanel();
}

void GamelistView::onThemeChanged(const std::shared_ptr<ThemeData>& theme)
{
    if (mLegacyMode) {
        legacyOnThemeChanged(theme);
        return;
    }

    using namespace ThemeFlags;

    // Remove old theme extras.
    for (auto extra : mThemeExtras) {
        removeChild(extra);
        delete extra;
    }
    mThemeExtras.clear();

    // Add new theme extras.
    mThemeExtras = ThemeData::makeExtras(theme, getName());
    for (auto extra : mThemeExtras)
        addChild(extra);

    mList.applyTheme(theme, getName(), "gamelist", ALL);

    // TODO: Implement logic to populate component vectors.

    sortChildren();
}

void GamelistView::update(int deltaTime)
{
    if (mLegacyMode) {
        legacyUpdate(deltaTime);
        return;
    }

    updateChildren(deltaTime);
}

void GamelistView::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {parentTrans * getTransform()};

    float scaleX {trans[0].x};
    float scaleY {trans[1].y};

    glm::ivec2 pos {static_cast<int>(std::round(trans[3].x)),
                    static_cast<int>(std::round(trans[3].y))};
    glm::ivec2 size {static_cast<int>(std::round(mSize.x * scaleX)),
                     static_cast<int>(std::round(mSize.y * scaleY))};

    Renderer::pushClipRect(pos, size);
    renderChildren(trans);
    Renderer::popClipRect();
}

HelpStyle GamelistView::getHelpStyle()
{
    HelpStyle style;
    style.applyTheme(mTheme, getName());
    return style;
}

std::vector<HelpPrompt> GamelistView::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;

    if (Settings::getInstance()->getBool("QuickSystemSelect") &&
        SystemData::sSystemVector.size() > 1)
        prompts.push_back(HelpPrompt("left/right", "system"));

    if (mRoot->getSystem()->getThemeFolder() == "custom-collections" && mCursorStack.empty() &&
        ViewController::getInstance()->getState().viewing == ViewController::GAMELIST)
        prompts.push_back(HelpPrompt("a", "enter"));
    else
        prompts.push_back(HelpPrompt("a", "launch"));

    prompts.push_back(HelpPrompt("b", "back"));
    prompts.push_back(HelpPrompt("x", "view media"));

    if (!UIModeController::getInstance()->isUIModeKid())
        prompts.push_back(HelpPrompt("back", "options"));
    if (mRoot->getSystem()->isGameSystem() && Settings::getInstance()->getBool("RandomAddButton"))
        prompts.push_back(HelpPrompt("thumbstickclick", "random"));

    if (mRoot->getSystem()->getThemeFolder() == "custom-collections" &&
        !CollectionSystemsManager::getInstance()->isEditing() && mCursorStack.empty() &&
        ViewController::getInstance()->getState().viewing == ViewController::GAMELIST &&
        ViewController::getInstance()->getState().viewstyle != ViewController::BASIC) {
        prompts.push_back(HelpPrompt("y", "jump to game"));
    }
    else if (mRoot->getSystem()->isGameSystem() &&
             (mRoot->getSystem()->getThemeFolder() != "custom-collections" ||
              !mCursorStack.empty()) &&
             !UIModeController::getInstance()->isUIModeKid() &&
             !UIModeController::getInstance()->isUIModeKiosk() &&
             (Settings::getInstance()->getBool("FavoritesAddButton") ||
              CollectionSystemsManager::getInstance()->isEditing())) {
        std::string prompt = CollectionSystemsManager::getInstance()->getEditingCollection();
        prompts.push_back(HelpPrompt("y", prompt));
    }
    else if (mRoot->getSystem()->isGameSystem() &&
             mRoot->getSystem()->getThemeFolder() == "custom-collections" &&
             CollectionSystemsManager::getInstance()->isEditing()) {
        std::string prompt = CollectionSystemsManager::getInstance()->getEditingCollection();
        prompts.push_back(HelpPrompt("y", prompt));
    }
    return prompts;
}

void GamelistView::updateInfoPanel()
{
    FileData* file {(mList.size() == 0 || mList.isScrolling()) ? nullptr : mList.getSelected()};

    // If the game data has already been rendered to the info panel, then skip it this time.
    if (file == mLastUpdated)
        return;

    if (!mList.isScrolling())
        mLastUpdated = file;

    bool hideMetaDataFields {false};

    if (file) {
        // Always hide the metadata fields if browsing grouped custom collections.
        if (file->getSystem()->isCustomCollection() &&
            file->getPath() == file->getSystem()->getName())
            hideMetaDataFields = true;
        else
            hideMetaDataFields = (file->metadata.get("hidemetadata") == "true");

        // Always hide the metadata fields for placeholders as well.
        if (file->getType() == PLACEHOLDER) {
            hideMetaDataFields = true;
            mLastUpdated = nullptr;
        }
    }

    // TODO: Implement gamelist logic.
}
