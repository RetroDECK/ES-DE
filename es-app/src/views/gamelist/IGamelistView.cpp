//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  IGamelistView.cpp
//
//  Interface that defines the minimum for a GamelistView.
//

#include "views/gamelist/IGamelistView.h"

#include "Sound.h"
#include "Window.h"
#include "guis/GuiGamelistOptions.h"
#include "views/UIModeController.h"
#include "views/ViewController.h"

IGamelistView::IGamelistView(Window* window, FileData* root)
    : GuiComponent(window)
    , mRoot(root)
{
    setSize(static_cast<float>(Renderer::getScreenWidth()),
            static_cast<float>(Renderer::getScreenHeight()));
}

void IGamelistView::setTheme(const std::shared_ptr<ThemeData>& theme)
{
    mTheme = theme;
    onThemeChanged(theme);
}

bool IGamelistView::input(InputConfig* config, Input input)
{
    // Select button opens GuiGamelistOptions.
    if (!UIModeController::getInstance()->isUIModeKid() && // Line break.
        config->isMappedTo("back", input) && input.value) {
        ViewController::getInstance()->cancelViewTransitions();
        stopListScrolling();
        mWindow->pushGui(new GuiGamelistOptions(mWindow, this->mRoot->getSystem()));
        return true;
    }

    // Ctrl-R reloads the view when debugging.
    else if (Settings::getInstance()->getBool("Debug") &&
             config->getDeviceId() == DEVICE_KEYBOARD &&
             (SDL_GetModState() & (KMOD_LCTRL | KMOD_RCTRL)) && input.id == SDLK_r &&
             input.value != 0) {
        LOG(LogDebug) << "IGamelistView::input(): Reloading view";
        ViewController::getInstance()->reloadGamelistView(this, true);
        return true;
    }

    return GuiComponent::input(config, input);
}

HelpStyle IGamelistView::getHelpStyle()
{
    HelpStyle style;
    style.applyTheme(mTheme, getName());
    return style;
}

void IGamelistView::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans{parentTrans * getTransform()};

    float scaleX = trans[0].x;
    float scaleY = trans[1].y;

    glm::ivec2 pos{static_cast<int>(std::round(trans[3].x)),
                   static_cast<int>(std::round(trans[3].y))};
    glm::ivec2 size{static_cast<int>(std::round(mSize.x * scaleX)),
                    static_cast<int>(std::round(mSize.y * scaleY))};

    Renderer::pushClipRect(pos, size);
    renderChildren(trans);
    Renderer::popClipRect();
}
