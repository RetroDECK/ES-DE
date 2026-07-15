//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiRomMSync.h
//
//  Modal dialog wrapper around RomMLibrarySync for the manual "FORCE FULL RESYNC" menu
//  action: shows a busy animation and blocks input while syncing, then reports the result.
//

#ifndef ES_APP_GUIS_GUI_ROMM_SYNC_H
#define ES_APP_GUIS_GUI_ROMM_SYNC_H

#include "GuiComponent.h"
#include "RomM/RomMLibrarySync.h"
#include "components/BusyComponent.h"

#include <memory>

// Input is blocked while syncing, matching the existing GuiGameImporter/GuiApplicationUpdater
// pattern for background-thread-driven dialogs. The actual fetch/apply logic lives in
// RomMLibrarySync, shared with the silent background sync triggered on startup.
class GuiRomMSync : public GuiComponent
{
public:
    // forceFullResync: forwarded to RomMLibrarySync - see its constructor for details. Also
    // switches the busy-animation text to reflect that a full resync is running.
    explicit GuiRomMSync(bool forceFullResync = false);
    ~GuiRomMSync();

    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;
    bool input(InputConfig* config, Input input) override;
    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    Renderer* mRenderer;
    BusyComponent mBusyAnim;
    std::unique_ptr<RomMLibrarySync> mSync;
};

#endif // ES_APP_GUIS_GUI_ROMM_SYNC_H
