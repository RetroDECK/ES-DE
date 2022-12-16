#  SPDX-License-Identifier: MIT
#
#  EmulationStation Desktop Edition
#  CMakeLists.txt (es-core)
#
#  CMake configuration for es-core.
#

project(core)

set(CORE_HEADERS
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AsyncHandle.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AudioManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CECInput.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GuiComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HelpStyle.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HttpReq.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ImageIO.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/InputConfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/InputManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Log.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MameNames.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Settings.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Sound.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ThemeData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Window.h

    # Animations
    ${CMAKE_CURRENT_SOURCE_DIR}/src/animations/Animation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/animations/AnimationController.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/animations/LambdaAnimation.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/animations/MoveCameraAnimation.h

    # Primary GUI components
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/primary/CarouselComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/primary/GridComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/primary/PrimaryComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/primary/TextListComponent.h

    # Secondary GUI components
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/AnimatedImageComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/BadgeComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/BusyComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/ButtonComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/ComponentGrid.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/ComponentList.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/DateTimeComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/DateTimeEditComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/FlexboxComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/GameSelectorComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/GIFAnimComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/HelpComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/IList.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/ImageComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/LottieAnimComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/MenuComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/NinePatchComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/OptionListComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/RatingComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/ScrollableContainer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/ScrollIndicatorComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/SliderComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/SwitchComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/TextComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/TextEditComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/VideoComponent.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/VideoFFmpegComponent.h

    # GUIs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiDetectDevice.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiInfoPopup.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiInputConfig.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiMsgBox.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiTextEditKeyboardPopup.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiTextEditPopup.h

    # Renderers
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderers/Renderer.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderers/RendererOpenGL.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderers/ShaderOpenGL.h

    # Resources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/Font.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/ResourceManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/TextureData.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/TextureDataManager.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/TextureResource.h

    # Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/CImgUtil.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/FileSystemUtil.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/MathUtil.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/PlatformUtil.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/StringUtil.h
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/TimeUtil.h
)

set(CORE_SOURCES
    ${CMAKE_CURRENT_SOURCE_DIR}/src/AudioManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/CECInput.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/GuiComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HelpStyle.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/HttpReq.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ImageIO.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/InputConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/InputManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Log.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/MameNames.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Scripting.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Settings.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Sound.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/ThemeData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/Window.cpp

    # Animations
    ${CMAKE_CURRENT_SOURCE_DIR}/src/animations/AnimationController.cpp

    # Secondary GUI components
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/AnimatedImageComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/BadgeComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/BusyComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/ButtonComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/ComponentGrid.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/ComponentList.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/DateTimeComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/DateTimeEditComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/FlexboxComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/GIFAnimComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/HelpComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/ImageComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/LottieAnimComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/MenuComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/NinePatchComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/RatingComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/ScrollableContainer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/SliderComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/SwitchComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/TextComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/TextEditComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/VideoComponent.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/components/VideoFFmpegComponent.cpp

    # GUIs
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiDetectDevice.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiInfoPopup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiInputConfig.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiMsgBox.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiTextEditKeyboardPopup.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/guis/GuiTextEditPopup.cpp

    # Renderer
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderers/Renderer.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderers/RendererOpenGL.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/renderers/ShaderOpenGL.cpp

    # Resources
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/Font.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/ResourceManager.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/TextureResource.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/TextureData.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/resources/TextureDataManager.cpp

    # Utils
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/CImgUtil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/FileSystemUtil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/MathUtil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/PlatformUtil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/StringUtil.cpp
    ${CMAKE_CURRENT_SOURCE_DIR}/src/utils/TimeUtil.cpp
)

#---------------------------------------------------------------------------------------------------
# Miscellaneous configuration.

include_directories(${COMMON_INCLUDE_DIRS})
add_library(es-core STATIC ${CORE_SOURCES} ${CORE_HEADERS})
target_link_libraries(es-core ${COMMON_LIBRARIES})
