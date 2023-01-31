//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GameSelectorComponent.h
//
//  Makes a selection of games based on theme-controlled criteria.
//

#ifndef ES_CORE_COMPONENTS_GAME_SELECTOR_COMPONENT_H
#define ES_CORE_COMPONENTS_GAME_SELECTOR_COMPONENT_H

#include "GuiComponent.h"
#include "Log.h"
#include "Settings.h"
#include "ThemeData.h"

class GameSelectorComponent : public GuiComponent
{
public:
    GameSelectorComponent(SystemData* system)
        : mSystem {system}
        , mGameSelection {GameSelection::RANDOM}
        , mNeedsRefresh {false}
        , mGameCount {1}
        , mAllowDuplicates {false}
    {
        mSystem->getRootFolder()->setUpdateListCallback([&]() { mNeedsRefresh = true; });
    }

    ~GameSelectorComponent()
    {
        if (std::find(SystemData::sSystemVector.cbegin(), SystemData::sSystemVector.cend(),
                      mSystem) != SystemData::sSystemVector.cend() ||
            (SystemData::sSystemVector.size() != 0 && mSystem->isGroupedCustomCollection()))
            mSystem->getRootFolder()->setUpdateListCallback(nullptr);
    }

    enum class GameSelection {
        RANDOM,
        LAST_PLAYED,
        MOST_PLAYED
    };

    const std::vector<FileData*>& getGames() const { return mGames; }
    void setNeedsRefresh() { mNeedsRefresh = true; }
    const bool getNeedsRefresh() { return mNeedsRefresh; }
    const GameSelection getGameSelection() const { return mGameSelection; }
    const std::string& getSelectorName() const { return mSelectorName; }
    const int getGameCount() const { return mGameCount; }

    void refreshGames()
    {
        if (!mNeedsRefresh)
            return;

        FileData* lastGame {nullptr};

        if (mGameCount == 1 && !mGames.empty())
            lastGame = mGames.front();

        mGames.clear();
        mNeedsRefresh = false;

        bool isKidMode {(Settings::getInstance()->getString("UIMode") == "kid" ||
                         Settings::getInstance()->getBool("ForceKid"))};

        if (mGameSelection == GameSelection::RANDOM) {
            int tries {mSystem->getRootFolder()->getGameCount().first < 6 ? 12 : 8};
            for (int i {0}; i < mGameCount; ++i) {
                if (mSystem->getRootFolder()->getGameCount().first == 0)
                    break;
                if (!mAllowDuplicates &&
                    mSystem->getRootFolder()->getGameCount().first == mGames.size())
                    break;
                FileData* randomGame {nullptr};

                if (mGameCount > 1 || lastGame == nullptr ||
                    mSystem->getRootFolder()->getGameCount().first == 1)
                    randomGame = mSystem->getRandomGame(nullptr, true);
                else
                    randomGame = mSystem->getRandomGame(lastGame, true);

                if (std::find(mGames.begin(), mGames.end(), randomGame) != mGames.end()) {
                    if (tries > 0) {
                        --i;
                        --tries;
                    }
                    else if (mAllowDuplicates && randomGame != nullptr) {
                        mGames.emplace_back(randomGame);
                    }

                    continue;
                }

                if (randomGame != nullptr)
                    mGames.emplace_back(randomGame);
            }
        }
        else if (mGameSelection == GameSelection::LAST_PLAYED) {
            for (auto& child : mSystem->getRootFolder()->getChildrenLastPlayed()) {
                if (child->getType() != GAME)
                    continue;
                if (!child->getCountAsGame())
                    continue;
                if (isKidMode && !child->getKidgame())
                    continue;
                if (child->metadata.get("lastplayed") == "0")
                    continue;
                mGames.emplace_back(child);
                if (static_cast<int>(mGames.size()) == mGameCount)
                    break;
            }
        }
        else if (mGameSelection == GameSelection::MOST_PLAYED) {
            for (auto& child : mSystem->getRootFolder()->getChildrenMostPlayed()) {
                if (child->getType() != GAME)
                    continue;
                if (!child->getCountAsGame())
                    continue;
                if (isKidMode && !child->getKidgame())
                    continue;
                if (child->metadata.get("playcount") == "0")
                    continue;
                mGames.emplace_back(child);
                if (static_cast<int>(mGames.size()) == mGameCount)
                    break;
            }
        }
    }

    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties)
    {
        const ThemeData::ThemeElement* elem {theme->getElement(view, element, "gameselector")};
        if (!elem)
            return;

        // Remove the leading "gameselector_" part of the element string in order to directly
        // match with the optional "gameselector" property used in other elements.
        mSelectorName = element.substr(13, std::string::npos);

        if (elem->has("selection")) {
            const std::string& selection {elem->get<std::string>("selection")};
            if (selection == "random") {
                mGameSelection = GameSelection::RANDOM;
            }
            else if (selection == "lastplayed") {
                mGameSelection = GameSelection::LAST_PLAYED;
                mSystem->getRootFolder()->setUpdateChildrenLastPlayed(true);
                mSystem->getRootFolder()->updateLastPlayedList();
            }
            else if (selection == "mostplayed") {
                mGameSelection = GameSelection::MOST_PLAYED;
                mSystem->getRootFolder()->setUpdateChildrenMostPlayed(true);
                mSystem->getRootFolder()->updateMostPlayedList();
            }
            else {
                mGameSelection = GameSelection::RANDOM;
                LOG(LogWarning) << "GameSelectorComponent: Invalid theme configuration, property "
                                   "\"selection\" for element \""
                                << element.substr(13) << "\" defined as \"" << selection << "\"";
            }
        }

        if (elem->has("gameCount"))
            mGameCount = glm::clamp(static_cast<int>(elem->get<unsigned int>("gameCount")), 1, 30);

        if (elem->has("allowDuplicates"))
            mAllowDuplicates = elem->get<bool>("allowDuplicates");
    }

private:
    SystemData* mSystem;
    std::vector<FileData*> mGames;

    std::string mSelectorName;
    GameSelection mGameSelection;
    bool mNeedsRefresh;
    int mGameCount;
    bool mAllowDuplicates;
};

#endif // ES_CORE_COMPONENTS_GAME_SELECTOR_COMPONENT_H
