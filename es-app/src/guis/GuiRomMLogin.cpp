//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiRomMLogin.cpp
//

#include "guis/GuiRomMLogin.h"

#include "RomM/RomMCache.h"
#include "RomM/RomMLocalFavorites.h"
#include "RomM/RomMUtils.h"
#include "Settings.h"
#include "Window.h"
#include "guis/GuiRomMPairing.h"
#include "guis/GuiSettings.h"
#include "utils/LocalizationUtil.h"

void GuiRomMLogin::push(Window* window,
                        unsigned int menuColorPrimary,
                        const std::function<void(const std::string&)>& onPaired,
                        const std::function<void()>& onLoggedOut)
{
    auto s = new GuiSettings(_("LOGIN"));

    const bool isLoggedIn {RomMUtils::isLoggedIn()};

    // Fixed while logged in - changing it out from under an active pairing would leave
    // RomMToken silently associated with the wrong server.
    std::shared_ptr<TextComponent> rommServerURL;
    if (isLoggedIn) {
        auto serverURLDisplay = std::make_shared<TextComponent>(
            Settings::getInstance()->getString("RomMServerURL"), Font::get(FONT_SIZE_MEDIUM),
            menuColorPrimary, ALIGN_RIGHT);
        s->addWithLabel(_("SERVER URL"), serverURLDisplay);
    }
    else {
        rommServerURL = std::make_shared<TextComponent>("", Font::get(FONT_SIZE_MEDIUM),
                                                        menuColorPrimary, ALIGN_RIGHT);
        s->addEditableTextComponent(_("SERVER URL"), rommServerURL,
                                    Settings::getInstance()->getString("RomMServerURL"));
        rommServerURL->setSize(0.0f, rommServerURL->getFont()->getHeight());
        s->addSaveFunc([rommServerURL, s] {
            if (rommServerURL->getValue() != Settings::getInstance()->getString("RomMServerURL")) {
                Settings::getInstance()->setString("RomMServerURL", rommServerURL->getValue());
                s->setNeedsSaving();
            }
        });
    }

    if (isLoggedIn) {
        ComponentListRow logoutRow;
        logoutRow.addElement(std::make_shared<TextComponent>(
                                 _("LOG OUT"), Font::get(FONT_SIZE_MEDIUM), menuColorPrimary),
                             true);
        logoutRow.makeAcceptInputHandler([s, onLoggedOut] {
            Settings::getInstance()->setString("RomMToken", "");
            Settings::getInstance()->setString("RomMTokenExpiresAt", "");
            Settings::getInstance()->saveFile();
            RomMCache::getInstance().clearAll();
            RomMCache::getInstance().flush();
            RomMLocalFavorites::getInstance().clearAll();
            // Copy onLoggedOut to the stack before deleting s - this lambda's captures live
            // inside s's own ComponentList, so delete s would free onLoggedOut mid-call.
            const auto onLoggedOutCopy = onLoggedOut;
            // Delete before invoking the callback - it may push its own GUI (e.g.
            // noGamesDialog()), which deleting afterward would immediately tear down again.
            delete s;
            if (onLoggedOutCopy)
                onLoggedOutCopy();
        });
        s->addRow(logoutRow);
    }
    else {
        ComponentListRow pairRow;
        pairRow.addElement(std::make_shared<TextComponent>(
                               _("START PAIR"), Font::get(FONT_SIZE_MEDIUM), menuColorPrimary),
                           true);
        pairRow.makeAcceptInputHandler([window, s, rommServerURL, onPaired] {
            auto onSuccess = [s, rommServerURL, onPaired](const std::string& resolvedUrl) {
                rommServerURL->setValue(resolvedUrl);
                // Delete before invoking the callback - it may push its own GUI (e.g.
                // noGamesDialog()), which deleting afterward would immediately tear down again.
                delete s;
                if (onPaired)
                    onPaired(resolvedUrl);
            };
            window->pushGui(new GuiRomMPairing(rommServerURL->getValue(), onSuccess));
        });
        s->addRow(pairRow);
    }

    window->pushGui(s);
}
