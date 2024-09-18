//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  ComponentList.cpp
//
//  Used to lay out and navigate lists in GUI menus.
//

#include "components/ComponentList.h"

#include "resources/Font.h"
#include "utils/LocalizationUtil.h"

#define TOTAL_HORIZONTAL_PADDING_PX 20.0f

ComponentList::ComponentList()
    : IList<ComponentListRow, void*> {LIST_SCROLL_STYLE_SLOW, ListLoopType::LIST_NEVER_LOOP}
    , mRenderer {Renderer::getInstance()}
    , mFocused {false}
    , mSetupCompleted {false}
    , mBottomCameraOffset {false}
    , mSingleRowScroll {false}
    , mRowHeight {std::round(Font::get(FONT_SIZE_MEDIUM)->getHeight())}
    , mSelectorBarOffset {0.0f}
    , mCameraOffset {0.0f}
    , mScrollIndicatorStatus {SCROLL_NONE}
{
    // Adjust the padding relative to the aspect ratio and screen resolution to make it look
    // coherent regardless of screen type. The 1.778 aspect ratio value is the 16:9 reference.
    const float aspectValue {1.778f / mRenderer->getScreenAspectRatio()};
    mHorizontalPadding =
        TOTAL_HORIZONTAL_PADDING_PX * aspectValue * mRenderer->getScreenWidthModifier();

    if (mRenderer->getIsVerticalOrientation())
        mHorizontalPadding *= 0.7f;
}

void ComponentList::addRow(const ComponentListRow& row, bool setCursorHere)
{
    IList<ComponentListRow, void*>::Entry entry;
    entry.object = nullptr;
    entry.data = row;

    this->add(entry);

    for (auto it = mEntries.back().data.elements.cbegin();
         it != mEntries.back().data.elements.cend(); ++it)
        addChild(it->component.get());

    updateElementSize(mEntries.back().data);
    updateElementPosition(mEntries.back().data);

    if (setCursorHere) {
        mCursor = static_cast<int>(mEntries.size()) - 1;
        onCursorChanged(CursorState::CURSOR_STOPPED);
    }
}

void ComponentList::onSizeChanged()
{
    for (auto it = mEntries.cbegin(); it != mEntries.cend(); ++it) {
        updateElementSize(it->data);
        updateElementPosition(it->data);
    }

    updateCameraOffset();
}

bool ComponentList::input(InputConfig* config, Input input)
{
    if (size() == 0)
        return false;

    mSingleRowScroll = false;

    if (input.value &&
        (config->isMappedTo("a", input) || config->isMappedLike("lefttrigger", input) ||
         config->isMappedLike("righttrigger", input))) {
        stopScrolling();
    }

    // Give it to the current row's input handler.
    if (mEntries.at(mCursor).data.inputHandler) {
        if (mEntries.at(mCursor).data.inputHandler(config, input))
            return true;
    }
    else {
        // No input handler assigned, do the default, which is to give it
        // to the rightmost element in the row.
        auto& row = mEntries.at(mCursor).data;
        if (row.elements.size()) {
            if (row.elements.back().component->input(config, input))
                return true;
        }
    }

    // Input handler didn't consume the input - try to scroll.
    if (config->isMappedLike("up", input)) {
        mSingleRowScroll = true;
        return listInput(input.value != 0 ? -1 : 0);
    }
    else if (config->isMappedLike("down", input)) {
        mSingleRowScroll = true;
        return listInput(input.value != 0 ? 1 : 0);
    }
    else if (config->isMappedLike("leftshoulder", input)) {
        return listInput(input.value != 0 ? -6 : 0);
    }
    else if (config->isMappedLike("rightshoulder", input)) {
        return listInput(input.value != 0 ? 6 : 0);
    }
    else if (config->isMappedLike("lefttrigger", input)) {
        if (input.value != 0) {
            mSelectorBarOffset = 0;
            return listFirstRow();
        }
    }
    else if (config->isMappedLike("righttrigger", input)) {
        if (input.value != 0) {
            mSelectorBarOffset = mEntries.size() - 1.0f;
            return listLastRow();
        }
    }

    return false;
}

void ComponentList::update(int deltaTime)
{
    // Scroll indicator logic, used by ScrollIndicatorComponent.
    bool scrollIndicatorChanged {false};

    if (getTotalRowHeight() > mSize.y) {
        if (mCameraOffset == 0) {
            if (mScrollIndicatorStatus != SCROLL_DOWN) {
                mScrollIndicatorStatus = SCROLL_DOWN;
                scrollIndicatorChanged = true;
            }
        }
        else if (mBottomCameraOffset) {
            if (mScrollIndicatorStatus != SCROLL_UP) {
                mScrollIndicatorStatus = SCROLL_UP;
                scrollIndicatorChanged = true;
            }
        }
        else if (mCameraOffset > 0) {
            if (mScrollIndicatorStatus != SCROLL_UP_DOWN) {
                mScrollIndicatorStatus = SCROLL_UP_DOWN;
                scrollIndicatorChanged = true;
            }
        }
    }

    if (scrollIndicatorChanged == true && mScrollIndicatorChangedCallback != nullptr)
        mScrollIndicatorChangedCallback(mScrollIndicatorStatus, mSingleRowScroll);

    listUpdate(deltaTime);

    if (mFocused && size()) {
        // Update our currently selected row.
        for (auto it = mEntries.at(mCursor).data.elements.cbegin();
             it != mEntries.at(mCursor).data.elements.cend(); ++it) {
            it->component->update(deltaTime);
        }
    }
}

void ComponentList::onCursorChanged(const CursorState& state)
{
    mSetupCompleted = true;

    // Update the selector bar position.
    mSelectorBarOffset = 0;
    for (int i {0}; i < mCursor; ++i)
        mSelectorBarOffset += mRowHeight;

    updateCameraOffset();

    // This is terribly inefficient but we don't know what we came from so...
    if (size()) {
        for (auto it = mEntries.cbegin(); it != mEntries.cend(); ++it)
            it->data.elements.back().component->onFocusLost();

        mEntries.at(mCursor).data.elements.back().component->onFocusGained();
    }

    if (mCursorChangedCallback)
        mCursorChangedCallback(state);

    updateHelpPrompts();
}

void ComponentList::updateCameraOffset()
{
    const float oldCameraOffset {mCameraOffset};

    // Move the camera to scroll.
    const float totalHeight {getTotalRowHeight()};
    if (totalHeight > mSize.y) {
        const float target {mSelectorBarOffset + mRowHeight / 2.0f - (mSize.y / 2.0f)};

        // Clamp the camera to prevent a fraction of a row from being displayed.
        mCameraOffset = 0.0f;
        unsigned int i {0};
        while (mCameraOffset < target && i < mEntries.size()) {
            mCameraOffset += mRowHeight;
            if (mCameraOffset >= totalHeight - mSize.y) {
                if (mSetupCompleted) {
                    if (mScrollIndicatorStatus == ComponentList::SCROLL_NONE &&
                        oldCameraOffset == 0.0f)
                        break;
                    if (mScrollIndicatorStatus != ComponentList::SCROLL_NONE &&
                        oldCameraOffset == 0.0f)
                        mBottomCameraOffset = true;
                    else if (mCameraOffset != oldCameraOffset)
                        mBottomCameraOffset = true;
                }
                break;
            }
            ++i;
        }

        if (mCameraOffset < oldCameraOffset &&
            (oldCameraOffset > mSelectorBarOffset ||
             mScrollIndicatorStatus != ComponentList::SCROLL_NONE))
            mBottomCameraOffset = false;

        if (mCameraOffset < 0.0f)
            mCameraOffset = 0.0f;
    }
    else {
        mCameraOffset = 0.0f;
    }
}

void ComponentList::render(const glm::mat4& parentTrans)
{
    if (!size())
        return;

    glm::mat4 trans {parentTrans * getTransform()};

    // Clip everything to be inside our bounds.
    glm::vec3 dim {mSize.x, mSize.y, 0.0f};
    dim.x = (trans[0].x * dim.x + trans[3].x) - trans[3].x;
    dim.y = (trans[1].y * dim.y + trans[3].y) - trans[3].y;

    const int clipRectPosX {static_cast<int>(std::round(trans[3].x))};
    const int clipRectPosY {static_cast<int>(std::round(trans[3].y))};
    const int clipRectSizeX {static_cast<int>(std::round(dim.x))};
    const int clipRectSizeY {static_cast<int>(std::round(dim.y) + 1.0f)};

    mRenderer->pushClipRect(glm::ivec2 {clipRectPosX, clipRectPosY},
                            glm::ivec2 {clipRectSizeX, clipRectSizeY});

    // Move camera the scroll distance.
    trans = glm::translate(trans, glm::vec3 {0.0f, -mCameraOffset, 0.0f});

    const bool darkColorScheme {Settings::getInstance()->getString("MenuColorScheme") != "light"};

    // Draw selector bar if we're using the dark color scheme.
    if (mFocused && mOpacity == 1.0f && darkColorScheme) {
        mRenderer->setMatrix(trans);
        mRenderer->drawRect(0.0f, mSelectorBarOffset, mSize.x, mRowHeight, mMenuColorSelector,
                            mMenuColorSelector, false, mOpacity, mDimming);
    }

    // Draw our entries.
    std::vector<GuiComponent*> drawAfterCursor;
    bool drawAll {false};
    for (size_t i {0}; i < mEntries.size(); ++i) {
        auto& entry = mEntries.at(i);
        drawAll = !mFocused || i != static_cast<unsigned int>(mCursor);
        for (auto it = entry.data.elements.cbegin(); it != entry.data.elements.cend(); ++it) {
            if (drawAll || it->invertWhenSelected) {
                // For the row where the cursor is at, we want to remove any hue from the
                // font or image before inverting, as it would otherwise lead to an ugly
                // inverted color (e.g. red inverting to a green hue).
                if (mFocused && i == static_cast<size_t>(mCursor) &&
                    it->component->getValue() != "") {
                    // Check if we're dealing with text or an image component.
                    bool isTextComponent {true};
                    unsigned int origColor {it->component->getColor()};
                    if (origColor == 0) {
                        origColor = it->component->getColorShift();
                        isTextComponent = false;
                    }
                    // Check if the color is neutral.
                    unsigned char byteRed {static_cast<unsigned char>(origColor >> 24 & 0xFF)};
                    unsigned char byteGreen {static_cast<unsigned char>(origColor >> 16 & 0xFF)};
                    unsigned char byteBlue {static_cast<unsigned char>(origColor >> 8 & 0xFF)};
                    // If it's neutral, just proceed with normal rendering.
                    if (byteRed == byteGreen && byteGreen == byteBlue) {
                        it->component->render(trans);
                    }
                    else {
                        if (isTextComponent)
                            it->component->setColor(mMenuColorPrimary);
                        else
                            it->component->setColorShift(mMenuColorPrimary);
                        it->component->render(trans);
                        // Revert to the original color after rendering.
                        if (isTextComponent)
                            it->component->setColor(origColor);
                        else
                            it->component->setColorShift(origColor);
                    }
                }
                else {
                    it->component->render(trans);
                }
            }
            else {
                drawAfterCursor.push_back(it->component.get());
            }
        }
    }

    mRenderer->setMatrix(trans);

    // Draw selector bar if we're using the light color scheme.
    if (mFocused && !darkColorScheme) {
        if (mOpacity == 1.0f) {
            mRenderer->drawRect(0.0f, mSelectorBarOffset, mSize.x, mRowHeight, mMenuColorSelector,
                                mMenuColorSelector, false, mOpacity, mDimming,
                                Renderer::BlendFactor::ONE_MINUS_DST_COLOR,
                                Renderer::BlendFactor::ZERO);

            mRenderer->drawRect(0.0f, mSelectorBarOffset, mSize.x, mRowHeight, 0x777777FF,
                                0x777777FF, false, mOpacity, mDimming, Renderer::BlendFactor::ONE,
                                Renderer::BlendFactor::ONE);
        }

        for (auto it = drawAfterCursor.cbegin(); it != drawAfterCursor.cend(); ++it)
            (*it)->render(trans);

        // Reset matrix if one of these components changed it.
        if (drawAfterCursor.size())
            mRenderer->setMatrix(trans);
    }

    // Draw separators.
    float offsetY {0.0f};
    for (unsigned int i {0}; i < mEntries.size(); ++i) {
        mRenderer->drawRect(0.0f, offsetY, mSize.x, 1.0f * mRenderer->getScreenResolutionModifier(),
                            mMenuColorSeparators, mMenuColorSeparators, false, mOpacity, mDimming);
        offsetY += mRowHeight;
    }

    mRenderer->drawRect(0.0f, offsetY, mSize.x, 1.0f * mRenderer->getScreenResolutionModifier(),
                        mMenuColorSeparators, mMenuColorSeparators, false, mOpacity, mDimming);
    mRenderer->popClipRect();
}

void ComponentList::updateElementPosition(const ComponentListRow& row)
{
    float yOffset {0.0f};
    for (auto it = mEntries.cbegin(); it != mEntries.cend() && &it->data != &row; ++it)
        yOffset += mRowHeight;

    // Assume updateElementSize has already been called.
    float offsetX {mHorizontalPadding / 2.0f};

    for (unsigned int i {0}; i < row.elements.size(); ++i) {
        const auto comp = row.elements.at(i).component;

        // Center vertically.
        comp->setPosition(offsetX, (mRowHeight - std::floor(comp->getSize().y)) / 2.0f + yOffset);
        offsetX += comp->getSize().x;
    }
}

void ComponentList::updateElementSize(const ComponentListRow& row)
{
    float width {mSize.x - mHorizontalPadding};
    std::vector<std::shared_ptr<GuiComponent>> resizeVec;

    for (auto it = row.elements.cbegin(); it != row.elements.cend(); ++it) {
        if (it->resizeWidth)
            resizeVec.push_back(it->component);
        else
            width -= it->component->getSize().x;
    }

    // This can happen if the element has zero width, or close to zero width.
    if (width < 0.0f)
        width = 0.0f;

    // Redistribute the "unused" width equally among the components if resizeWidth is set to true.
    width = width / resizeVec.size();
    for (auto it = resizeVec.cbegin(); it != resizeVec.cend(); ++it)
        (*it)->setSize(width, (*it)->getSize().y);
}

void ComponentList::textInput(const std::string& text, const bool pasting)
{
    if (!size())
        return;

    mEntries.at(mCursor).data.elements.back().component->textInput(text, pasting);
}

std::vector<HelpPrompt> ComponentList::getHelpPrompts()
{
    if (!size())
        return std::vector<HelpPrompt>();

    std::vector<HelpPrompt> prompts {
        mEntries.at(mCursor).data.elements.back().component->getHelpPrompts()};

    if (size() > 1) {
        bool addMovePrompt {true};
        for (auto it = prompts.cbegin(); it != prompts.cend(); ++it) {
            if (it->first == "up/down" || it->first == "up/down/left/right") {
                addMovePrompt = false;
                break;
            }
        }
        if (addMovePrompt)
            prompts.push_back(HelpPrompt("up/down", _("choose")));
    }

    return prompts;
}

bool ComponentList::moveCursor(int amount)
{
    const bool returnValue {listInput(amount)};
    listInput(0);
    return returnValue;
}
