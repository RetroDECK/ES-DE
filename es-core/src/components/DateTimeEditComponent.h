//  SPDX-License-Identifier: MIT
//
//  ES-DE Frontend
//  DateTimeEditComponent.h
//
//  Date and time edit component.
//

#ifndef ES_CORE_COMPONENTS_DATE_TIME_EDIT_COMPONENT_H
#define ES_CORE_COMPONENTS_DATE_TIME_EDIT_COMPONENT_H

#include "GuiComponent.h"
#include "components/TextComponent.h"
#include "renderers/Renderer.h"
#include "utils/TimeUtil.h"

class DateTimeEditComponent : public GuiComponent
{
public:
    DateTimeEditComponent(bool alignRight = false);

    void onSizeChanged() override;

    void setValue(const std::string& val) override;
    std::string getValue() const override { return mTime; }
    unsigned int getColor() const override { return mColor; }

    bool input(InputConfig* config, Input input) override;
    void update(int deltaTime) override;
    void render(const glm::mat4& parentTrans) override;

    // Text color.
    void setColor(unsigned int color) override;

    void setOriginalColor(unsigned int color) override { mColorOriginalValue = color; }
    void setChangedColor(unsigned int color) override { mColorChangedValue = color; }

    void setFont(std::shared_ptr<Font> font);
    void setUppercase(bool uppercase);

    std::vector<HelpPrompt> getHelpPrompts() override;

private:
    std::string getDisplayString() const;

    void changeDate();
    void updateText();

    Renderer* mRenderer;
    Utils::Time::DateTime mTime;
    Utils::Time::DateTime mTimeBeforeEdit;

    bool mEditing;
    int mEditIndex;

    int mKeyRepeatDir;
    int mKeyRepeatTimer;

    std::unique_ptr<TextComponent> mDateText;
    std::vector<glm::vec4> mCursorBoxes;

    unsigned int mColor;
    Utils::Time::DateTime mOriginalValue;
    unsigned int mColorOriginalValue;
    unsigned int mColorChangedValue;

    bool mAlignRight;
    bool mUppercase;
    bool mAutoSize;
};

#endif // ES_CORE_COMPONENTS_DATE_TIME_COMPONENT_H
