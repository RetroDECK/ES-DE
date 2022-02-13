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
#include "ThemeData.h"

class GameSelectorComponent : public GuiComponent
{
public:
    GameSelectorComponent(SystemData* system)
        : mSystem {system}
        , mGameSelection {GameSelection::RANDOM}
        , mGameCount {1}
    {
    }

    const std::vector<FileData*>& getGames() const { return mGames; }

    void refreshGames()
    {
        mGames.clear();

        bool isKidMode {(Settings::getInstance()->getString("UIMode") == "kid" ||
                         Settings::getInstance()->getBool("ForceKid"))};

        if (mGameSelection == GameSelection::RANDOM) {
            for (int i = 0; i < mGameCount; ++i) {
                FileData* randomGame {mSystem->getRandomGame()};
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

        if (elem->has("selection")) {
            const std::string selection {elem->get<std::string>("selection")};
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
                                   "<selection> set to \""
                                << selection << "\"";
            }
        }

        if (elem->has("count"))
            mGameCount = glm::clamp(static_cast<int>(elem->get<unsigned int>("count")), 1, 30);
    }

private:
    enum class GameSelection {
        RANDOM, // Replace with AllowShortEnumsOnASingleLine: false (clang-format >=11.0).
        LAST_PLAYED,
        MOST_PLAYED
    };

    SystemData* mSystem;
    std::vector<FileData*> mGames;

    GameSelection mGameSelection;
    int mGameCount;
};

#endif // ES_CORE_COMPONENTS_GAME_SELECTOR_COMPONENT_H
