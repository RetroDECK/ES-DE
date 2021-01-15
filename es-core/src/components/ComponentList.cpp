//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  ComponentList.cpp
//
//  Used to lay out and navigate lists in GUI menus.
//

#include "components/ComponentList.h"

#define TOTAL_HORIZONTAL_PADDING_PX 20.0f

ComponentList::ComponentList(Window* window) : IList<ComponentListRow,
        void*>(window, LIST_SCROLL_STYLE_SLOW, LIST_NEVER_LOOP)
{
    mSelectorBarOffset = 0;
    mCameraOffset = 0;
    mFocused = false;
}

void ComponentList::addRow(const ComponentListRow& row, bool setCursorHere)
{
    IList<ComponentListRow, void*>::Entry e;
    e.name = "";
    e.object = nullptr;
    e.data = row;

    this->add(e);

    for (auto it = mEntries.back().data.elements.cbegin();
            it != mEntries.back().data.elements.cend(); it++)
        addChild(it->component.get());

    updateElementSize(mEntries.back().data);
    updateElementPosition(mEntries.back().data);

    if (setCursorHere) {
        mCursor = static_cast<int>(mEntries.size()) - 1;
        onCursorChanged(CURSOR_STOPPED);
    }
}

void ComponentList::onSizeChanged()
{
    for (auto it = mEntries.cbegin(); it != mEntries.cend(); it++) {
        updateElementSize(it->data);
        updateElementPosition(it->data);
    }

    updateCameraOffset();
}

void ComponentList::onFocusLost()
{
    mFocused = false;
}

void ComponentList::onFocusGained()
{
    mFocused = true;
}

bool ComponentList::input(InputConfig* config, Input input)
{
    if (size() == 0)
        return false;

    // Give it to the current row's input handler.
    if (mEntries.at(mCursor).data.input_handler) {
        if (mEntries.at(mCursor).data.input_handler(config, input))
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
        return listInput(input.value != 0 ? -1 : 0);
    }
    else if (config->isMappedLike("down", input)) {
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
    listUpdate(deltaTime);

    if (size()) {
        // Update our currently selected row.
        for (auto it = mEntries.at(mCursor).data.elements.cbegin();
                it != mEntries.at(mCursor).data.elements.cend(); it++)
            it->component->update(deltaTime);
    }
}

void ComponentList::onCursorChanged(const CursorState& state)
{
    // Update the selector bar position.
    // In the future this might be animated.
    mSelectorBarOffset = 0;
    for (int i = 0; i < mCursor; i++)
        mSelectorBarOffset += getRowHeight(mEntries.at(i).data);

    updateCameraOffset();

    // This is terribly inefficient but we don't know what we came from so...
    if (size()) {
        for (auto it = mEntries.cbegin(); it != mEntries.cend(); it++)
            it->data.elements.back().component->onFocusLost();

        mEntries.at(mCursor).data.elements.back().component->onFocusGained();
    }

    if (mCursorChangedCallback)
        mCursorChangedCallback(state);

    updateHelpPrompts();
}

void ComponentList::updateCameraOffset()
{
    // Move the camera to scroll.
    const float totalHeight = getTotalRowHeight();
    if (totalHeight > mSize.y()) {
        float target = mSelectorBarOffset + getRowHeight(mEntries.at(mCursor).data)/2 -
                (mSize.y() / 2);

        // Clamp it.
        mCameraOffset = 0;
        unsigned int i = 0;
        while (mCameraOffset < target && i < mEntries.size()) {
            mCameraOffset += getRowHeight(mEntries.at(i).data);
            i++;
        }

        if (mCameraOffset < 0)
            mCameraOffset = 0;
        else if (mCameraOffset + mSize.y() > totalHeight)
            mCameraOffset = totalHeight - mSize.y();
    }
    else {
        mCameraOffset = 0;
    }
}

void ComponentList::render(const Transform4x4f& parentTrans)
{
    if (!size())
        return;

    Transform4x4f trans = parentTrans * getTransform();

    // Clip everything to be inside our bounds.
    Vector3f dim(mSize.x(), mSize.y(), 0);
    dim = trans * dim - trans.translation();
    Renderer::pushClipRect(Vector2i(static_cast<int>(trans.translation().x()),
            static_cast<int>(trans.translation().y())), Vector2i(static_cast<int>(
            std::round(dim.x())), static_cast<int>(std::round(dim.y() + 1))));

    // Scroll the camera.
    trans.translate(Vector3f(0, -std::round(mCameraOffset), 0));

    // Draw our entries.
    std::vector<GuiComponent*> drawAfterCursor;
    bool drawAll;
    for (unsigned int i = 0; i < mEntries.size(); i++) {
        auto& entry = mEntries.at(i);
        drawAll = !mFocused || i != static_cast<unsigned int>(mCursor);
        for (auto it = entry.data.elements.cbegin(); it != entry.data.elements.cend(); it++) {
            if (drawAll || it->invert_when_selected) {
                // For the row where the cursor is at, we want to remove any hue from the
                // font or image before inverting, as it would otherwise lead to an ugly
                // inverted color (e.g. red inverting to a green hue).
                if (i == mCursor && it->component->getValue() != "" ) {
                    // Check if we're dealing with text or an image component.
                    bool isTextComponent = true;
                    unsigned int origColor = it->component->getColor();
                    if (origColor == 0) {
                        origColor = it->component->getColorShift();
                        isTextComponent = false;
                    }
                    // Check if the color is neutral.
                    unsigned char byteRed = origColor >> 24 & 0xFF;
                    unsigned char byteGreen = origColor >> 16 & 0xFF;
                    unsigned char byteBlue = origColor >> 8 & 0xFF;
                    // If it's neutral, just proceed with normal rendering.
                    if (byteRed == byteGreen && byteGreen == byteBlue) {
                        it->component->render(trans);
                    }
                    else {
                        // Note: I've disabled this code as it's overly complicated,
                        // instead we're now using simple constants which should be
                        // good enough. Let's keep the code though if needed in the
                        // future for some reason.
//                        // If there is a hue, average the brightness values to make
//                        // an equivalent gray value before inverting.
//                        // This is not the proper way to do a BW conversion as the RGB values
//                        // should not be evenly distributed, but it's definitely good enough
//                        // for this situation.
//                        unsigned char byteAverage = (byteRed + byteGreen + byteBlue) / 3;
//                        unsigned int averageColor = byteAverage << 24 | byteAverage << 16 |
//                                byteAverage << 8 | 0xFF;
                        if (isTextComponent)
                            it->component->setColor(DEFAULT_INVERTED_TEXTCOLOR);
                        else
                            it->component->setColorShift(DEFAULT_INVERTED_IMAGECOLOR);

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

    // Custom rendering.
    Renderer::setMatrix(trans);

    float opacity = mOpacity / 255.0f;

    // Draw selector bar.
    if (mFocused) {
        // Inversion: src * (1 - dst) + dst * 0 = where src = 1
        // Need a function that goes roughly 0x777777 -> 0xFFFFFF
        // and 0xFFFFFF -> 0x777777
        // (1 - dst) + 0x77

        const float selectedRowHeight = getRowHeight(mEntries.at(mCursor).data);

        if (opacity == 1) {
            Renderer::drawRect(0.0f, mSelectorBarOffset, mSize.x(), selectedRowHeight,
                    0xFFFFFFFF, 0xFFFFFFFF, false, opacity, trans,
                    Renderer::Blend::ONE_MINUS_DST_COLOR, Renderer::Blend::ZERO);

            Renderer::drawRect(0.0f, mSelectorBarOffset, mSize.x(), selectedRowHeight,
                    0x777777FF, 0x777777FF, false, opacity, trans,
                    Renderer::Blend::ONE, Renderer::Blend::ONE);
            // Hack to draw 2px dark on left/right of the bar.
            Renderer::drawRect(0.0f, mSelectorBarOffset, 2.0f, selectedRowHeight,
                    0x878787FF, 0x878787FF, false, opacity, trans);
            Renderer::drawRect(mSize.x() - 2.0f, mSelectorBarOffset, 2.0f, selectedRowHeight,
                    0x878787FF, 0x878787FF, false, opacity, trans);
        }

        for (auto it = drawAfterCursor.cbegin(); it != drawAfterCursor.cend(); it++)
            (*it)->render(trans);

        // Reset matrix if one of these components changed it.
        if (drawAfterCursor.size())
            Renderer::setMatrix(trans);
    }

    // Draw separators.
    float y = 0;
    for (unsigned int i = 0; i < mEntries.size(); i++) {
        Renderer::drawRect(0.0f, y, mSize.x(), 1.0f * Renderer::getScreenHeightModifier(),
                0xC6C7C6FF, 0xC6C7C6FF, false, opacity, trans);
        y += getRowHeight(mEntries.at(i).data);
    }

    Renderer::drawRect(0.0f, y, mSize.x(), 1.0f * Renderer::getScreenHeightModifier(),
            0xC6C7C6FF, 0xC6C7C6FF, false, opacity, trans);
    Renderer::popClipRect();
}

float ComponentList::getRowHeight(const ComponentListRow& row) const
{
    // Returns the highest component height found in the row.
    float height = 0;
    for (unsigned int i = 0; i < row.elements.size(); i++) {
        if (row.elements.at(i).component->getSize().y() > height)
            height = row.elements.at(i).component->getSize().y();
    }

    return height;
}

float ComponentList::getTotalRowHeight() const
{
    float height = 0;
    for (auto it = mEntries.cbegin(); it != mEntries.cend(); it++)
        height += getRowHeight(it->data);

    return height;
}

void ComponentList::updateElementPosition(const ComponentListRow& row)
{
    float yOffset = 0;
    for (auto it = mEntries.cbegin(); it != mEntries.cend() && &it->data != &row; it++)
        yOffset += getRowHeight(it->data);

    // Assumes updateElementSize has already been called.
    float rowHeight = getRowHeight(row);

    float x = (TOTAL_HORIZONTAL_PADDING_PX * Renderer::getScreenWidthModifier()) / 2;
    for (unsigned int i = 0; i < row.elements.size(); i++) {
        const auto comp = row.elements.at(i).component;

        // Center vertically.
        comp->setPosition(x, (rowHeight - comp->getSize().y()) / 2 + yOffset);
        x += comp->getSize().x();
    }
}

void ComponentList::updateElementSize(const ComponentListRow& row)
{
    float width = mSize.x() - (TOTAL_HORIZONTAL_PADDING_PX * Renderer::getScreenWidthModifier());
    std::vector< std::shared_ptr<GuiComponent> > resizeVec;

    for (auto it = row.elements.cbegin(); it != row.elements.cend(); it++) {
        if (it->resize_width)
            resizeVec.push_back(it->component);
        else
            width -= it->component->getSize().x();
    }

    // Redistribute the "unused" width equally among the components with resize_width set to true.
    width = width / resizeVec.size();
    for (auto it = resizeVec.cbegin(); it != resizeVec.cend(); it++)
        (*it)->setSize(width, (*it)->getSize().y());
}

void ComponentList::textInput(const std::string& text)
{
    if (!size())
        return;

    mEntries.at(mCursor).data.elements.back().component->textInput(text);
}

std::vector<HelpPrompt> ComponentList::getHelpPrompts()
{
    if (!size())
        return std::vector<HelpPrompt>();

    std::vector<HelpPrompt> prompts =
            mEntries.at(mCursor).data.elements.back().component->getHelpPrompts();

    if (size() > 1) {
        bool addMovePrompt = true;
        for (auto it = prompts.cbegin(); it != prompts.cend(); it++) {
            if (it->first == "up/down" || it->first == "up/down/left/right") {
                addMovePrompt = false;
                break;
            }
        }
        if (addMovePrompt)
            prompts.push_back(HelpPrompt("up/down", "choose"));
    }

    return prompts;
}

bool ComponentList::moveCursor(int amt)
{
    bool ret = listInput(amt);
    listInput(0);
    return ret;
}
