//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiRomMSync.cpp
//

#include "guis/GuiRomMSync.h"

#include "Window.h"
#include "guis/GuiMsgBox.h"
#include "utils/LocalizationUtil.h"
#include "utils/StringUtil.h"

GuiRomMSync::GuiRomMSync(bool forceFullResync)
    : mRenderer {Renderer::getInstance()}
    , mSync {std::make_unique<RomMLibrarySync>(forceFullResync)}
{
    setSize(mRenderer->getScreenWidth() * 0.4f, mRenderer->getScreenHeight() * 0.1f);
    setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                (mRenderer->getScreenHeight() - mSize.y) / 2.0f);

    mBusyAnim.setSize(mSize);
    mBusyAnim.setText(forceFullResync ? _("PERFORMING FULL ROMM RESYNC...") :
                                        _("SYNCING ROMM LIBRARY..."));
    mBusyAnim.onSizeChanged();

    mSync->start();
}

GuiRomMSync::~GuiRomMSync() {}

void GuiRomMSync::update(int deltaTime)
{
    if (!mSync->isDone()) {
        mBusyAnim.update(deltaTime);
    }
    else {
        mSync->applyResults();

        const std::string message {
            Utils::String::format(_("ROMM SYNC COMPLETE\n%d GAME(S) ADDED\n%d GAME(S) REMOVED"),
                                  mSync->getAddedCount(), mSync->getRemovedCount())};
        mWindow->pushGui(new GuiMsgBox(message));
        delete this;
        return;
    }

    GuiComponent::update(deltaTime);
}

void GuiRomMSync::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {parentTrans * getTransform()};
    renderChildren(trans);

    if (!mSync->isDone())
        mBusyAnim.render(trans);
}

bool GuiRomMSync::input(InputConfig* config, Input input)
{
    // Block all input while syncing, matching GuiGameImporter's behavior for its own
    // background-thread-driven inventory step.
    return true;
}

std::vector<HelpPrompt> GuiRomMSync::getHelpPrompts() { return std::vector<HelpPrompt>(); }
