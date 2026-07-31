//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  GuiRomMPairing.cpp
//

#include "guis/GuiRomMPairing.h"

#include "Settings.h"
#include "Window.h"
#include "components/ImageComponent.h"
#include "components/TextComponent.h"
#include "guis/GuiMsgBox.h"
#include "resources/Font.h"
#include "utils/LocalizationUtil.h"
#include "utils/QrCodeUtil.h"
#include "utils/StringUtil.h"

GuiRomMPairing::GuiRomMPairing(const std::string& rawServerUrl,
                               const std::function<void(const std::string&)>& onSuccess)
    : mRenderer {Renderer::getInstance()}
    , mOnSuccess {onSuccess}
    , mFlow {std::make_unique<RomMDeviceAuthFlow>(rawServerUrl)}
    , mCancelling {false}
    , mApprovalContentBuilt {false}
    , mGrid {glm::ivec2 {1, 4}}
{
    const float aspectValue {1.778f / mRenderer->getScreenAspectRatio()};
    const float width {glm::clamp(0.60f * aspectValue, 0.50f,
                                  (mRenderer->getIsVerticalOrientation() ? 0.85f : 0.80f)) *
                       mRenderer->getScreenWidth()};
    setSize(width, mRenderer->getScreenHeight() * 0.75f);
    setPosition((mRenderer->getScreenWidth() - mSize.x) / 2.0f,
                (mRenderer->getScreenHeight() - mSize.y) / 2.0f);

    mBusyAnim.setSize(mSize);
    mBusyAnim.setText(_("CONNECTING TO ROMM SERVER..."));
    mBusyAnim.onSizeChanged();

    mFlow->start();
}

GuiRomMPairing::~GuiRomMPairing() {}

void GuiRomMPairing::buildApprovalContent()
{
    if (mApprovalContentBuilt)
        return;
    mApprovalContentBuilt = true;

    addChild(&mBackground);
    mBackground.fitTo(mSize);

    mTitle = std::make_shared<TextComponent>(_("PAIR WITH ROMM SERVER"), Font::get(FONT_SIZE_LARGE),
                                             mMenuColorTitle, ALIGN_CENTER);
    mGrid.setEntry(mTitle, glm::ivec2 {0, 0}, false, true, glm::ivec2 {1, 1},
                   GridFlags::BORDER_BOTTOM);

    mInstructions = std::make_shared<TextComponent>(
        _("SCAN THE QR CODE OR OPEN THE URL BELOW, THEN APPROVE THE REQUEST"),
        Font::get(FONT_SIZE_SMALL), mMenuColorSecondary, ALIGN_CENTER);
    mGrid.setEntry(mInstructions, glm::ivec2 {0, 1}, false, true);

    const float qrSizePx {std::min(mSize.x, mSize.y) * 0.45f};
    mQrImage = std::make_shared<ImageComponent>();
    mQrImage->setResize(qrSizePx, qrSizePx);
    std::vector<unsigned char> qrPixels;
    size_t qrPixelSize {0};
    if (Utils::QrCode::encodeToRgba(mFlow->getVerificationUrl(), 4, qrPixels, qrPixelSize)) {
        mQrImage->setRawImage(qrPixels.data(), qrPixelSize, qrPixelSize);
    }
    mGrid.setEntry(mQrImage, glm::ivec2 {0, 2}, false, false);

    mVerificationUrlText = std::make_shared<TextComponent>(
        mFlow->getVerificationUrl(), Font::get(FONT_SIZE_SMALL), mMenuColorPrimary, ALIGN_CENTER);
    mGrid.setEntry(mVerificationUrlText, glm::ivec2 {0, 3}, false, true);

    mGrid.setSize(mSize);
    mGrid.setRowHeightPerc(0, mTitle->getFont()->getHeight() / mSize.y);
    mGrid.setRowHeightPerc(1, mInstructions->getFont()->getHeight() * 2.0f / mSize.y);
    mGrid.setRowHeightPerc(2, (qrSizePx + 20.0f) / mSize.y);
    // Row 3 left unset so it absorbs the remaining height.

    addChild(&mGrid);
}

void GuiRomMPairing::finishOnMainThread()
{
    const RomMDeviceAuthFlow::State state {mFlow->getState()};

    if (state == RomMDeviceAuthFlow::State::Cancelled) {
        delete this;
        return;
    }

    if (state == RomMDeviceAuthFlow::State::Success) {
        const std::string resolvedServerUrl {mFlow->getResolvedServerUrl()};
        Settings::getInstance()->setString("RomMServerURL", resolvedServerUrl);
        Settings::getInstance()->setString("RomMToken", mFlow->getAccessToken());
        Settings::getInstance()->setString("RomMTokenExpiresAt", mFlow->getExpiresAt());
        Settings::getInstance()->setString(
            "RomMTokenScopes", Utils::String::vectorToDelimitedString(mFlow->getScopes(), ","));
        Settings::getInstance()->saveFile();
        const std::function<void(const std::string&)> onSuccess {mOnSuccess};
        // Delete before invoking the callback, matching GuiMsgBox::deleteMeAndCall()'s pattern -
        // ~GuiComponent() removes this from the window's gui stack, so a callback that tears
        // down the whole stack (e.g. GuiMenu::close(true)) won't double-delete this dialog.
        delete this;
        onSuccess(resolvedServerUrl);
        return;
    }

    Window* window {mWindow};
    std::string message;
    if (state == RomMDeviceAuthFlow::State::Denied)
        message = _("PAIRING WAS DENIED ON THE ROMM SERVER");
    else if (state == RomMDeviceAuthFlow::State::Expired)
        message = _("THE PAIRING CODE EXPIRED, PLEASE TRY AGAIN");
    else
        message = Utils::String::format(_("COULDN'T PAIR WITH THE ROMM SERVER:\n%s"),
                                        mFlow->getLastError().c_str());

    delete this;
    window->pushGui(new GuiMsgBox(message));
}

void GuiRomMPairing::update(int deltaTime)
{
    const RomMDeviceAuthFlow::State state {mFlow->getState()};

    if (state == RomMDeviceAuthFlow::State::Init) {
        mBusyAnim.update(deltaTime);
    }
    else if (state == RomMDeviceAuthFlow::State::AwaitingApproval) {
        buildApprovalContent();
        if (mCancelling)
            mInstructions->setText(_("CANCELLING..."));
    }
    else {
        finishOnMainThread();
        return;
    }

    GuiComponent::update(deltaTime);
}

void GuiRomMPairing::render(const glm::mat4& parentTrans)
{
    glm::mat4 trans {parentTrans * getTransform()};
    renderChildren(trans);

    if (mFlow->getState() == RomMDeviceAuthFlow::State::Init)
        mBusyAnim.render(trans);
}

bool GuiRomMPairing::input(InputConfig* config, Input input)
{
    if (input.value != 0 && !mCancelling &&
        mFlow->getState() == RomMDeviceAuthFlow::State::AwaitingApproval &&
        (config->isMappedTo("b", input) || config->isMappedTo("back", input))) {
        mCancelling = true;
        mFlow->cancel();
    }

    return true;
}

std::vector<HelpPrompt> GuiRomMPairing::getHelpPrompts()
{
    std::vector<HelpPrompt> prompts;
    if (mFlow->getState() == RomMDeviceAuthFlow::State::AwaitingApproval && !mCancelling)
        prompts.push_back(HelpPrompt("b", _("cancel")));
    return prompts;
}
