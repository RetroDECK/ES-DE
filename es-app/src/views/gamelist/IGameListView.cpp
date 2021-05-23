//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  IGameListView.cpp
//
//  Interface that defines the minimum for a GameListView.
//

#include "views/gamelist/IGameListView.h"

#include "guis/GuiGamelistOptions.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"
#include "AudioManager.h"
#include "Sound.h"
#include "Window.h"

bool IGameListView::input(InputConfig* config, Input input)
{
    // Select button opens GuiGamelistOptions.
    if (!UIModeController::getInstance()->isUIModeKid() &&
            config->isMappedTo("back", input) && input.value) {
        ViewController::get()->cancelViewTransitions();
        stopListScrolling();
        mWindow->pushGui(new GuiGamelistOptions(mWindow, this->mRoot->getSystem()));
        return true;
    }

    // Ctrl-R reloads the view when debugging.
    else if (Settings::getInstance()->getBool("Debug") &&
            config->getDeviceId() == DEVICE_KEYBOARD &&
            (SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL)) &&
            input.id == SDLK_r && input.value != 0) {
        LOG(LogDebug) << "IGameListView::input(): Reloading view";
        ViewController::get()->reloadGameListView(this, true);
        return true;
    }

    return GuiComponent::input(config, input);
}

void IGameListView::setTheme(const std::shared_ptr<ThemeData>& theme)
{
    mTheme = theme;
    onThemeChanged(theme);
}

HelpStyle IGameListView::getHelpStyle()
{
    HelpStyle style;
    style.applyTheme(mTheme, getName());
    return style;
}

void IGameListView::render(const Transform4x4f& parentTrans)
{
    Transform4x4f trans = parentTrans * getTransform();

    float scaleX = trans.r0().x();
    float scaleY = trans.r1().y();

    Vector2i pos(static_cast<int>(std::round(trans.translation()[0])),
            static_cast<int>(std::round(trans.translation()[1])));
    Vector2i size(static_cast<int>(std::round(mSize.x() * scaleX)),
            static_cast<int>(std::round(mSize.y() * scaleY)));

    Renderer::pushClipRect(pos, size);
    renderChildren(trans);
    Renderer::popClipRect();
}
