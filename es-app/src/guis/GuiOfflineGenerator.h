//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiOfflineGenerator.h
//
//  User interface for the miximage offline generator.
//  Calls MiximageGenerator to do the actual work.
//

#ifndef ES_APP_GUIS_GUI_OFFLINE_GENERATOR_H
#define ES_APP_GUIS_GUI_OFFLINE_GENERATOR_H

#include "GuiComponent.h"
#include "MiximageGenerator.h"
#include "components/ButtonComponent.h"
#include "components/ComponentGrid.h"
#include "views/ViewController.h"

#include <queue>

class TextComponent;

class GuiOfflineGenerator : public GuiComponent
{
public:
    GuiOfflineGenerator(const std::queue<FileData*>& gameQueue);
    ~GuiOfflineGenerator();

private:
    void onSizeChanged() override;
    void update(int deltaTime) override;

    std::vector<HelpPrompt> getHelpPrompts() override;
    HelpStyle getHelpStyle() override { return ViewController::getInstance()->getViewHelpStyle(); }

    std::queue<FileData*> mGameQueue;

    std::unique_ptr<MiximageGenerator> mMiximageGenerator;
    std::thread mMiximageGeneratorThread;
    std::promise<bool> mGeneratorPromise;
    std::future<bool> mGeneratorFuture;

    FileData* mGame;

    bool mProcessing;
    bool mPaused;
    bool mOverwriting;
    std::string mResultMessage;

    unsigned int mTotalGames;
    unsigned int mGamesProcessed;
    unsigned int mImagesGenerated;
    unsigned int mImagesOverwritten;
    unsigned int mGamesSkipped;
    unsigned int mGamesFailed;

    Renderer* mRenderer;
    NinePatchComponent mBackground;
    ComponentGrid mGrid;

    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextComponent> mStatus;
    std::shared_ptr<TextComponent> mGameCounter;

    std::shared_ptr<TextComponent> mGeneratedLbl;
    std::shared_ptr<TextComponent> mGeneratedVal;

    std::shared_ptr<TextComponent> mOverwrittenLbl;
    std::shared_ptr<TextComponent> mOverwrittenVal;

    std::shared_ptr<TextComponent> mSkippedLbl;
    std::shared_ptr<TextComponent> mSkippedVal;

    std::shared_ptr<TextComponent> mFailedLbl;
    std::shared_ptr<TextComponent> mFailedVal;

    std::shared_ptr<TextComponent> mProcessingLbl;
    std::shared_ptr<TextComponent> mProcessingVal;
    std::string mGameName;

    std::shared_ptr<TextComponent> mLastErrorLbl;
    std::shared_ptr<TextComponent> mLastErrorVal;

    std::shared_ptr<ComponentGrid> mButtonGrid;
    std::shared_ptr<ButtonComponent> mStartPauseButton;
    std::shared_ptr<ButtonComponent> mCloseButton;
};

#endif // ES_APP_GUIS_GUI_OFFLINE_GENERATOR_H
