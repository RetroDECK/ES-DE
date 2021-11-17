//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  OptionListComponent.h
//
//  Provides a list of option components.
//  Supports various types using templates.
//

#ifndef ES_CORE_COMPONENTS_OPTION_LIST_COMPONENT_H
#define ES_CORE_COMPONENTS_OPTION_LIST_COMPONENT_H

#define OPTIONLIST_REPEAT_START_DELAY 650
#define OPTIONLIST_REPEAT_SPEED 250 // Lower is faster.

#define CHECKED_PATH ":/graphics/checkbox_checked.svg"
#define UNCHECKED_PATH ":/graphics/checkbox_unchecked.svg"

#include "GuiComponent.h"
#include "Log.h"
#include "Window.h"

#include <algorithm>

// Used to display a list of options.
// Can select one or multiple options.
template <typename T> class OptionListComponent : public GuiComponent
{
public:
    OptionListComponent(Window* window,
                        const HelpStyle& helpstyle,
                        const std::string& name,
                        bool multiSelect = false,
                        bool multiExclusiveSelect = false,
                        bool multiShowTotal = false)
        : GuiComponent{window}
        , mHelpStyle{helpstyle}
        , mMultiSelect{multiSelect}
        , mMultiExclusiveSelect{multiExclusiveSelect}
        , mMultiShowTotal{multiShowTotal}
        , mKeyRepeat{false}
        , mKeyRepeatDir{0}
        , mKeyRepeatTimer{0}
        , mKeyRepeatStartDelay{OPTIONLIST_REPEAT_START_DELAY}
        , mKeyRepeatSpeed{OPTIONLIST_REPEAT_SPEED}
        , mName{name}
        , mText{window}
        , mLeftArrow{window}
        , mRightArrow{window}
    {
        auto font{Font::get(FONT_SIZE_MEDIUM, FONT_PATH_LIGHT)};
        mText.setFont(font);
        mText.setColor(0x777777FF);
        mText.setHorizontalAlignment(ALIGN_CENTER);
        addChild(&mText);

        mLeftArrow.setResize(0, mText.getFont()->getLetterHeight());
        mRightArrow.setResize(0, mText.getFont()->getLetterHeight());

        if (mMultiSelect) {
            mRightArrow.setImage(":/graphics/arrow.svg");
            addChild(&mRightArrow);
        }
        else {
            mLeftArrow.setImage(":/graphics/option_arrow.svg");
            mLeftArrow.setFlipX(true);
            addChild(&mLeftArrow);

            mRightArrow.setImage(":/graphics/option_arrow.svg");
            addChild(&mRightArrow);
        }

        setSize(mLeftArrow.getSize().x + mRightArrow.getSize().x, font->getHeight());
    }

    // Handles positioning/resizing of text and arrows.
    void onSizeChanged() override
    {
        if (mText.getFont()->getLetterHeight() != mLeftArrow.getSize().y ||
            mLeftArrow.getTexture()->getPendingRasterization())
            mLeftArrow.setResize(0, mText.getFont()->getLetterHeight());
        if (mText.getFont()->getLetterHeight() != mRightArrow.getSize().y ||
            mRightArrow.getTexture()->getPendingRasterization())
            mRightArrow.setResize(0, mText.getFont()->getLetterHeight());

        if (mSize.x < (mLeftArrow.getSize().x + mRightArrow.getSize().x)) {
            LOG(LogWarning) << "OptionListComponent too narrow";
        }

        mText.setSize(mSize.x - mLeftArrow.getSize().x - mRightArrow.getSize().x,
                      mText.getFont()->getHeight());

        // Position.
        mLeftArrow.setPosition(0.0f, std::round((mSize.y - mLeftArrow.getSize().y) / 2.0f));
        mText.setPosition(mLeftArrow.getPosition().x + mLeftArrow.getSize().x,
                          (mSize.y - mText.getSize().y) / 2.0f);
        mRightArrow.setPosition(mText.getPosition().x + mText.getSize().x,
                                std::round((mSize.y - mRightArrow.getSize().y) / 2.0f));
    }

    bool input(InputConfig* config, Input input) override
    {
        if (config->isMappedTo("a", input) && input.value) {
            // Ignore input if the component has been disabled.
            if (!mEnabled)
                return true;
            mKeyRepeatDir = 0;
            open();
            return true;
        }
        if (!mMultiSelect) {
            if (config->isMappedLike("left", input)) {
                if (input.value) {
                    if (mKeyRepeat) {
                        mKeyRepeatDir = -1;
                        mKeyRepeatTimer = -(mKeyRepeatStartDelay - mKeyRepeatSpeed);
                    }
                    // Ignore input if the component has been disabled.
                    if (!mEnabled)
                        return true;
                    // Move selection to previous.
                    unsigned int i = getSelectedId();
                    int next = static_cast<int>(i) - 1;
                    if (next < 0)
                        next += static_cast<int>(mEntries.size());

                    mEntries.at(i).selected = false;
                    mEntries.at(next).selected = true;
                    onSelectedChanged();
                    return true;
                }
                else {
                    mKeyRepeatDir = 0;
                }
            }
            else if (config->isMappedLike("right", input)) {
                if (input.value) {
                    if (mKeyRepeat) {
                        mKeyRepeatDir = 1;
                        mKeyRepeatTimer = -(mKeyRepeatStartDelay - mKeyRepeatSpeed);
                    }
                    // Ignore input if the component has been disabled.
                    if (!mEnabled)
                        return true;
                    // Move selection to next.
                    unsigned int i = getSelectedId();
                    int next = (i + 1) % mEntries.size();
                    mEntries.at(i).selected = false;
                    mEntries.at(next).selected = true;
                    onSelectedChanged();
                    return true;
                }
                else {
                    mKeyRepeatDir = 0;
                }
            }
            else if (input.value) {
                mKeyRepeatDir = 0;
            }
        }
        return GuiComponent::input(config, input);
    }

    std::vector<T> getSelectedObjects()
    {
        std::vector<T> ret;
        for (auto it = mEntries.cbegin(); it != mEntries.cend(); ++it) {
            if (it->selected)
                ret.push_back(it->object);
        }

        return ret;
    }

    T getSelected()
    {
        assert(mMultiSelect == false);
        auto selected = getSelectedObjects();
        assert(selected.size() == 1);
        return selected.at(0);
    }

    void add(const std::string& name, const T& obj, bool selected, float maxNameLength = 0.0f)
    {
        OptionListData e;
        e.name = name;
        e.object = obj;
        e.selected = selected;
        e.maxNameLength = maxNameLength;

        mEntries.push_back(e);

        onSelectedChanged();
    }

    bool selectEntry(unsigned int entry)
    {
        if (mEntries.empty() || entry > mEntries.size()) {
            return false;
        }
        else {
            mEntries.at(entry).selected = true;
            onSelectedChanged();
            return true;
        }
    }

    bool unselectEntry(unsigned int entry)
    {
        if (entry > mEntries.size()) {
            return false;
        }
        else {
            mEntries.at(entry).selected = false;
            onSelectedChanged();
            return true;
        }
    }

    void selectAll()
    {
        for (unsigned int i = 0; i < mEntries.size(); ++i)
            mEntries.at(i).selected = true;
        onSelectedChanged();
    }

    void selectNone()
    {
        for (unsigned int i = 0; i < mEntries.size(); ++i)
            mEntries.at(i).selected = false;
        onSelectedChanged();
    }

    void sortEntriesByName()
    {
        std::sort(std::begin(mEntries), std::end(mEntries),
                  [](OptionListData a, OptionListData b) { return a.name < b.name; });
    }

    unsigned int getSelectedId()
    {
        assert(mMultiSelect == false);
        for (unsigned int i = 0; i < mEntries.size(); ++i) {
            if (mEntries.at(i).selected)
                return i;
        }

        LOG(LogWarning) << "OptionListComponent::getSelectedId() - "
                           "no selected element found, defaulting to 0";
        return 0;
    }

    void setOverrideMultiText(const std::string& text) { mOverrideMultiText = text; }

    void setKeyRepeat(bool state,
                      int delay = OPTIONLIST_REPEAT_START_DELAY,
                      int speed = OPTIONLIST_REPEAT_SPEED)
    {
        mKeyRepeat = state;
        mKeyRepeatStartDelay = delay;
        mKeyRepeatSpeed = speed;
    }

    void update(int deltaTime) override
    {
        if (mKeyRepeat && mKeyRepeatDir != 0) {
            mKeyRepeatTimer += deltaTime;
            while (mKeyRepeatTimer >= mKeyRepeatSpeed) {
                if (mKeyRepeatDir == -1) {
                    // Move selection to previous.
                    unsigned int i = getSelectedId();
                    int next = static_cast<int>(i) - 1;
                    if (next < 0)
                        next += static_cast<int>(mEntries.size());
                    mEntries.at(i).selected = false;
                    mEntries.at(next).selected = true;
                    onSelectedChanged();
                }
                else {
                    // Move selection to next.
                    unsigned int i = getSelectedId();
                    int next = (i + 1) % mEntries.size();
                    mEntries.at(i).selected = false;
                    mEntries.at(next).selected = true;
                    onSelectedChanged();
                }
                mKeyRepeatTimer -= mKeyRepeatSpeed;
            }
        }

        GuiComponent::update(deltaTime);
    }

    HelpStyle getHelpStyle() override { return mHelpStyle; }

private:
    struct OptionListData {
        std::string name;
        T object;
        bool selected;
        float maxNameLength;
    };

    HelpStyle mHelpStyle;

    void open()
    {
        // Open the list popup.
        mWindow->pushGui(new OptionListPopup(mWindow, getHelpStyle(), this, mName));
    }

    void onSelectedChanged()
    {
        if (mMultiSelect) {
            // Display the selected entry.
            std::stringstream ss;

            // For special situations, allow the "selected" text to be overridden to a custom value.
            if (mOverrideMultiText != "")
                ss << mOverrideMultiText;
            else if (mMultiShowTotal)
                ss << getSelectedObjects().size() << " (OF " << mEntries.size() << ") SELECTED";
            else
                ss << getSelectedObjects().size() << " SELECTED";

            mText.setText(ss.str());
            mText.setSize(0, mText.getSize().y);
            setSize(mText.getSize().x + mRightArrow.getSize().x +
                        std::round(Font::get(FONT_SIZE_MEDIUM)->getLetterHeight() * 0.68f),
                    mText.getSize().y);
            if (mParent) // Hack since there's no "on child size changed" callback.
                mParent->onSizeChanged();
        }
        else {
            // Display the selected entry and left/right option arrows.
            for (auto it = mEntries.cbegin(); it != mEntries.cend(); ++it) {
                if (it->selected) {
                    if (it->maxNameLength > 0.0f &&
                        Font::get(FONT_SIZE_MEDIUM)->sizeText(Utils::String::toUpper(it->name)).x >
                            it->maxNameLength) {
                        // A maximum length parameter has been passed and the "name" size surpasses
                        // this value, so abbreviate the string inside the arrows.
                        auto font = Font::get(FONT_SIZE_MEDIUM);
                        // Calculate with an extra dot to give some leeway.
                        float dotsSize = font->sizeText("....").x;
                        std::string abbreviatedString = font->getTextMaxWidth(
                            Utils::String::toUpper(it->name), it->maxNameLength);
                        float sizeDifference = font->sizeText(Utils::String::toUpper(it->name)).x -
                                               font->sizeText(abbreviatedString).x;
                        if (sizeDifference > 0.0f) {
                            // It doesn't make sense to abbreviate if the number of pixels removed
                            // by the abbreviation is less or equal to the size of the three dots
                            // that would be appended to the string.
                            if (sizeDifference <= dotsSize) {
                                abbreviatedString = it->name;
                            }
                            else {
                                if (abbreviatedString.back() == ' ')
                                    abbreviatedString.pop_back();
                                abbreviatedString += "...";
                            }
                        }
                        mText.setText(Utils::String::toUpper(abbreviatedString));
                    }
                    else {
                        mText.setText(Utils::String::toUpper(it->name));
                    }

                    mText.setSize(0.0f, mText.getSize().y);
                    setSize(mText.getSize().x + mLeftArrow.getSize().x + mRightArrow.getSize().x +
                                std::round(Font::get(FONT_SIZE_MEDIUM)->getLetterHeight() * 0.68f),
                            mText.getSize().y);
                    if (mParent) // Hack since there's no "on child size changed" callback.
                        mParent->onSizeChanged();
                    break;
                }
            }
        }
        onSizeChanged();
    }

    std::vector<HelpPrompt> getHelpPrompts() override
    {
        std::vector<HelpPrompt> prompts;
        if (!mMultiSelect)
            prompts.push_back(HelpPrompt("left/right", "change value"));

        prompts.push_back(HelpPrompt("a", "select"));
        return prompts;
    }

    bool mMultiSelect;
    bool mMultiExclusiveSelect;
    bool mMultiShowTotal;
    bool mKeyRepeat;

    int mKeyRepeatDir;
    int mKeyRepeatTimer;
    int mKeyRepeatStartDelay;
    int mKeyRepeatSpeed;

    std::string mOverrideMultiText;
    std::string mName;

    TextComponent mText;
    ImageComponent mLeftArrow;
    ImageComponent mRightArrow;

    std::vector<OptionListData> mEntries;

    // Subclass to OptionListComponent.
    class OptionListPopup : public GuiComponent
    {
    public:
        OptionListPopup(Window* window,
                        const HelpStyle& helpstyle,
                        OptionListComponent<T>* parent,
                        const std::string& title)
            : GuiComponent(window)
            , mMenu(window, title.c_str())
            , mParent(parent)
            , mHelpStyle(helpstyle)
        {
            auto font = Font::get(FONT_SIZE_MEDIUM);
            ComponentListRow row;

            bool hasSelectedRow = false;

            // If the exclusive selection flag has been set, i.e. only a single row can be selected
            // at a time, then make sure to gray out and disable any non-selected rows.
            if (mParent->mMultiExclusiveSelect) {
                for (auto entry : mParent->mEntries) {
                    if (entry.selected == true) {
                        hasSelectedRow = true;
                        break;
                    }
                }
            }

            // For selecting all/none.
            std::vector<ImageComponent*> checkBoxes;
            std::vector<TextComponent*> textEntries;

            for (auto it = mParent->mEntries.begin(); it != mParent->mEntries.end(); ++it) {
                row.elements.clear();
                auto textComponent = std::make_shared<TextComponent>(
                    mWindow, Utils::String::toUpper(it->name), font, 0x777777FF);
                row.addElement(textComponent, true);

                if (mParent->mMultiExclusiveSelect && hasSelectedRow && !(*it).selected) {
                    textComponent.get()->setOpacity(DISABLED_OPACITY);
                    textComponent.get()->setEnabled(false);
                }

                OptionListData& e = *it;

                if (mParent->mMultiSelect) {
                    // Add checkbox.
                    auto checkbox = std::make_shared<ImageComponent>(mWindow);
                    checkbox->setImage(it->selected ? CHECKED_PATH : UNCHECKED_PATH);
                    checkbox->setResize(0, font->getLetterHeight());
                    row.addElement(checkbox, false);

                    if (mParent->mMultiExclusiveSelect && hasSelectedRow && !(*it).selected)
                        checkbox.get()->setOpacity(DISABLED_OPACITY);

                    // Input handler.
                    // Update checkbox state and selected value.
                    row.makeAcceptInputHandler([this, &e, checkbox] {
                        auto list = mMenu.getList();
                        int cursorId = list->getCursorId();
                        bool isEnabled = list->getChild(cursorId * 2)->getEnabled();

                        if (mParent->mMultiExclusiveSelect && !isEnabled)
                            return;

                        e.selected = !e.selected;
                        checkbox->setImage(e.selected ? CHECKED_PATH : UNCHECKED_PATH);
                        mParent->onSelectedChanged();

                        // When selecting a row and the exclusive selection flag has been set,
                        // gray out and disable all other rows.
                        if (mParent->mMultiExclusiveSelect) {
                            for (unsigned int i = 0; i < mParent->mEntries.size(); ++i) {

                                bool isSelected = mParent->mEntries[cursorId].selected;

                                for (unsigned int i = 0; i < list->getChildCount(); i += 2) {
                                    if (i == static_cast<unsigned int>(cursorId) * 2)
                                        continue;
                                    if (isSelected) {
                                        mEnabled = false;
                                        list->getChild(i)->setEnabled(false);
                                        list->getChild(i)->setOpacity(DISABLED_OPACITY);
                                        list->getChild(i + 1)->setOpacity(DISABLED_OPACITY);
                                    }
                                    else {
                                        mEnabled = true;
                                        list->getChild(i)->setEnabled(true);
                                        list->getChild(i)->setOpacity(255);
                                        list->getChild(i + 1)->setOpacity(255);
                                    }
                                }
                            }
                        }
                    });

                    // For selecting all/none.
                    checkBoxes.push_back(checkbox.get());
                    textEntries.push_back(textComponent.get());
                }
                else {
                    // Input handler for non-multiselect.
                    // Update selected value and close.
                    row.makeAcceptInputHandler([this, &e] {
                        mParent->mEntries.at(mParent->getSelectedId()).selected = false;
                        e.selected = true;
                        mParent->onSelectedChanged();
                        delete this;
                    });
                }

                // Also set cursor to this row if we're not multi-select and this row is selected.
                mMenu.addRow(row, (!mParent->mMultiSelect && it->selected), false);
            }

            mMenu.addButton("BACK", "back", [this] { delete this; });

            if (mParent->mMultiSelect) {
                if (!mParent->mMultiExclusiveSelect) {
                    mMenu.addButton("SELECT ALL", "select all", [this, checkBoxes] {
                        for (unsigned int i = 0; i < mParent->mEntries.size(); ++i) {
                            mParent->mEntries.at(i).selected = true;
                            checkBoxes.at(i)->setImage(CHECKED_PATH);
                        }
                        mParent->onSelectedChanged();
                    });
                }

                mMenu.addButton("SELECT NONE", "select none", [this, checkBoxes, textEntries] {
                    for (unsigned int i = 0; i < mParent->mEntries.size(); ++i) {
                        mParent->mEntries.at(i).selected = false;
                        checkBoxes.at(i)->setImage(UNCHECKED_PATH);
                        if (mParent->mMultiExclusiveSelect) {
                            checkBoxes.at(i)->setOpacity(255);
                            textEntries.at(i)->setOpacity(255);
                            textEntries.at(i)->setEnabled(true);
                        }
                    }
                    mParent->onSelectedChanged();
                });
            }

            mMenu.setPosition((Renderer::getScreenWidth() - mMenu.getSize().x) / 2.0f,
                              Renderer::getScreenHeight() * 0.13f);
            addChild(&mMenu);
        }

        bool input(InputConfig* config, Input input) override
        {
            if (config->isMappedTo("b", input) && input.value != 0) {
                delete this;
                return true;
            }

            return GuiComponent::input(config, input);
        }

        std::vector<HelpPrompt> getHelpPrompts() override
        {
            auto prompts = mMenu.getHelpPrompts();
            prompts.push_back(HelpPrompt("a", "select"));
            prompts.push_back(HelpPrompt("b", "back"));
            return prompts;
        }

        HelpStyle getHelpStyle() override { return mHelpStyle; }

    private:
        MenuComponent mMenu;
        OptionListComponent<T>* mParent;
        HelpStyle mHelpStyle;
    };
};

#endif // ES_CORE_COMPONENTS_OPTION_LIST_COMPONENT_H
