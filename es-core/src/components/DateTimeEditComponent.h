//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  DateTimeEditComponent.h
//
//  Date and time edit component.
//

#ifndef ES_CORE_COMPONENTS_DATE_TIME_EDIT_COMPONENT_H
#define ES_CORE_COMPONENTS_DATE_TIME_EDIT_COMPONENT_H

#include "GuiComponent.h"
#include "renderers/Renderer.h"
#include "utils/TimeUtil.h"

class TextCache;

// Used to enter or display a specific point in time.
class DateTimeEditComponent : public GuiComponent
{
public:
    enum DisplayMode {
        DISP_DATE,
        DISP_DATE_TIME,
        DISP_RELATIVE_TO_NOW
    };

    DateTimeEditComponent(bool alignRight = false, DisplayMode dispMode = DISP_DATE);

    void onSizeChanged() override;

    void setValue(const std::string& val) override;
    std::string getValue() const override { return mTime; }
    unsigned int getColor() const override { return mColor; }

    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

    // Set how the point in time will be displayed:
    //  * DISP_DATE - only display the date.
    //  * DISP_DATE_TIME - display both the date and the time on that date.
    //  * DISP_RELATIVE_TO_NOW - intelligently display the point in time relative to
    //    right now (e.g. "5 secs ago", "3 minutes ago", "1 day ago".
    //    Automatically updates as time marches on.
    // The initial value is DISP_DATE.
    void setDisplayMode(DisplayMode mode);

    // Text color.
    void setColor(unsigned int color) override;
    // Font to use. Default is Font::get(FONT_SIZE_MEDIUM).

    void setOriginalColor(unsigned int color) override { mColorOriginalValue = color; }
    void setChangedColor(unsigned int color) override { mColorChangedValue = color; }

    void setFont(std::shared_ptr<Font> font);
    // Force text to be uppercase when in DISP_RELATIVE_TO_NOW mode.
    void setUppercase(bool uppercase);

    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    std::shared_ptr<Font> getFont() const override;
    std::string getDisplayString(DisplayMode mode) const;
    DisplayMode getCurrentDisplayMode() const { return mDisplayMode; }

    void changeDate();
    void updateTextCache();

    Renderer* mRenderer;
    Utils::Time::DateTime mTime;
    Utils::Time::DateTime mTimeBeforeEdit;

    bool mEditing;
    int mEditIndex;
    DisplayMode mDisplayMode;

    int mKeyRepeatDir;
    int mKeyRepeatTimer;
    int mRelativeUpdateAccumulator;

    std::unique_ptr<TextCache> mTextCache;
    std::vector<glm::vec4> mCursorBoxes;

    unsigned int mColor;
    Utils::Time::DateTime mOriginalValue;
    unsigned int mColorOriginalValue;
    unsigned int mColorChangedValue;

    std::shared_ptr<Font> mFont;
    bool mAlignRight;
    bool mUppercase;
    bool mAutoSize;
};

#endif // ES_CORE_COMPONENTS_DATE_TIME_COMPONENT_H
