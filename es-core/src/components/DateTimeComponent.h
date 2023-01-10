//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  DateTimeComponent.h
//
//  Provides the date and time, in absolute (actual date) or relative
//  (delta from current date and time) form.
//  Used by the gamelist views.
//

#ifndef ES_CORE_COMPONENTS_DATE_TIME_COMPONENT_H
#define ES_CORE_COMPONENTS_DATE_TIME_COMPONENT_H

#include "TextComponent.h"
#include "utils/TimeUtil.h"

class ThemeData;

// Used to display date and time.
class DateTimeComponent : public TextComponent
{
public:
    DateTimeComponent();
    DateTimeComponent(const std::string& text,
                      const std::shared_ptr<Font>& font,
                      unsigned int color = 0x000000FF,
                      Alignment horizontalAlignment = ALIGN_LEFT,
                      glm::vec3 pos = {0.0f, 0.0f, 0.0f},
                      glm::vec2 size = {0.0f, 0.0f},
                      unsigned int bgcolor = 0x00000000);

    void render(const glm::mat4& parentTrans) override;

    void setValue(const std::string& val) override;
    std::string getValue() const override;

    void setFormat(const std::string& format);
    void setDisplayRelative(bool displayRelative);

    void applyTheme(const std::shared_ptr<ThemeData>& theme,
                    const std::string& view,
                    const std::string& element,
                    unsigned int properties) override;

protected:
    void onTextChanged() override;

private:
    std::string getDisplayString() const;

    std::string mDefaultValue;
    Utils::Time::DateTime mTime;
    std::string mFormat;
    bool mDisplayRelative;
};

#endif // ES_CORE_COMPONENTS_DATE_TIME_COMPONENT_H
