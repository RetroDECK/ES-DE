//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiRomMLogin.h
//
//  Shared "LOGIN" screen for RomM (server URL entry plus pairing, or LOG OUT if already
//  paired).
//

#ifndef ES_APP_GUIS_GUI_ROMM_LOGIN_H
#define ES_APP_GUIS_GUI_ROMM_LOGIN_H

#include <functional>
#include <string>

class Window;

namespace GuiRomMLogin
{
    // Pushes the RomM "LOGIN" screen onto the window. The screen (and any pairing dialog it
    // opens) is deleted internally before either callback runs, so callers only need to handle
    // whatever's above it on the window stack.
    // onPaired is called with the resolved server URL after a successful pairing.
    // onLoggedOut is called after the user logs out of an already-paired server.
    // menuColorPrimary/menuColorRed are the caller's current GuiComponent::mMenuColorPrimary/
    // mMenuColorRed (protected, so neither can be read directly from this free function).
    void push(Window* window,
              unsigned int menuColorPrimary,
              unsigned int menuColorRed,
              const std::function<void(const std::string&)>& onPaired,
              const std::function<void()>& onLoggedOut);
} // namespace GuiRomMLogin

#endif // ES_APP_GUIS_GUI_ROMM_LOGIN_H
