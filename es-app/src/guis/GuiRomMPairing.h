//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiRomMPairing.h
//
//  Modal dialog for the RomM device-pairing ("PAIR WITH SERVER") menu action. Drives a
//  RomMDeviceAuthFlow on a background thread. Modeled on GuiRomMSync/GuiRomMDownload's
//  background-thread + BusyComponent pattern.
//

#ifndef ES_APP_GUIS_GUI_ROMM_PAIRING_H
#define ES_APP_GUIS_GUI_ROMM_PAIRING_H

#include "GuiComponent.h"
#include "RomM/RomMDeviceAuthFlow.h"
#include "components/BackgroundComponent.h"
#include "components/BusyComponent.h"
#include "components/ComponentGrid.h"

#include <functional>
#include <memory>
#include <string>

class ImageComponent;
class TextComponent;

class GuiRomMPairing : public GuiComponent
{
public:
    // onSuccess is called on the main thread with the resolved server URL once a successful
    // pairing has been acknowledged.
    explicit GuiRomMPairing(const std::string& rawServerUrl,
                            const std::function<void(const std::string&)>& onSuccess = nullptr);
    ~GuiRomMPairing();

    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;
    bool input(InputConfig* config, Input input) override;
    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    void buildApprovalContent(); // idempotent
    void finishOnMainThread();

    Renderer* mRenderer;
    std::function<void(const std::string&)> mOnSuccess;
    std::unique_ptr<RomMDeviceAuthFlow> mFlow;
    bool mCancelling;
    bool mApprovalContentBuilt;

    BusyComponent mBusyAnim;
    BackgroundComponent mBackground;
    ComponentGrid mGrid;
    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextComponent> mInstructions;
    std::shared_ptr<ImageComponent> mQrImage;
    std::shared_ptr<TextComponent> mVerificationUrlText;
};

#endif // ES_APP_GUIS_GUI_ROMM_PAIRING_H
