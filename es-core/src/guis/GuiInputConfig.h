//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GuiInputConfig.h
//
//  Input device configuration GUI (for keyboards, joysticks and gamepads).
//

#ifndef ES_CORE_GUIS_GUI_INPUT_CONFIG_H
#define ES_CORE_GUIS_GUI_INPUT_CONFIG_H

#include "GuiComponent.h"
#include "components/ComponentGrid.h"
#include "components/NinePatchComponent.h"

class ComponentList;
class TextComponent;

class GuiInputConfig : public GuiComponent
{
public:
    GuiInputConfig(InputConfig* target,
                   bool reconfigureAll,
                   const std::function<void()>& okCallback);

    void populateConfigList();

    void update(int deltaTime) override;
    void onSizeChanged() override;

private:
    // Move cursor to the next row if we're configuring all, or come out
    // of "configure mode" if we were only configuring one row.
    void rowDone();

    // Set text to "msg" + not greyed out.
    void error(const std::shared_ptr<TextComponent>& text, const std::string& msg);

    // Set text to "PRESS ANYTHING" + not greyed out.
    void setPress(const std::shared_ptr<TextComponent>& text);
    // Set text to -NOT DEFINED- + greyed out.
    void setNotDefined(const std::shared_ptr<TextComponent>& text);
    // Set text to "BUTTON 2"/"AXIS 2+", etc.
    void setAssignedTo(const std::shared_ptr<TextComponent>& text, Input input);

    bool assign(Input input, int inputId);
    void clearAssignment(int inputId);

    NinePatchComponent mBackground;
    ComponentGrid mGrid;

    std::shared_ptr<TextComponent> mTitle;
    std::shared_ptr<TextComponent> mSubtitle1;
    std::shared_ptr<TextComponent> mSubtitle2;
    std::shared_ptr<ComponentList> mList;
    std::vector<std::shared_ptr<TextComponent>> mMappings;
    std::shared_ptr<ComponentGrid> mButtonGrid;

    InputConfig* mTargetConfig;
    // Next input captured by mList will be interpretted as a remap.
    bool mConfiguringRow;
    // Move the cursor down after configuring a row and start configuring
    // the next row until we reach the bottom.
    bool mConfiguringAll;

    bool mHoldingInput;
    Input mHeldInput;
    int mHeldTime;
    int mHeldInputId;
};

#endif // ES_CORE_GUIS_GUI_INPUT_CONFIG_H
