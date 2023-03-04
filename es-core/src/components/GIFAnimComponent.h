//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  GIFAnimComponent.h
//
//  Component to play GIF animations.
//

#ifndef ES_CORE_COMPONENTS_GIF_ANIM_COMPONENT_H
#define ES_CORE_COMPONENTS_GIF_ANIM_COMPONENT_H

#include "GuiComponent.h"
#include "ThemeData.h"
#include "renderers/Renderer.h"
#include "resources/TextureResource.h"
#include "utils/MathUtil.h"

#include <FreeImage.h>
#include <chrono>

class GIFAnimComponent : public GuiComponent
{
public:
    GIFAnimComponent();
    ~GIFAnimComponent();

    void setAnimation(const std::string& path);
    void setPauseAnimation(bool state) { mExternalPause = state; }

    void resetFileAnimation() override;
    void onSizeChanged() override;

    virtual void applyTheme(const std::shared_ptr<ThemeData>& theme,
                            const std::string& view,
                            const std::string& element,
                            unsigned int properties) override;

    void update(int deltaTime) override;

private:
    void render(const glm::mat4& parentTrans) override;

    static inline unsigned int readProc(void* buffer,
                                        unsigned int size,
                                        unsigned int count,
                                        fi_handle handle)
    {
        return static_cast<unsigned int>(
            fread(buffer, size, count, reinterpret_cast<FILE*>(handle)));
    }

    static inline unsigned int writeProc(void* buffer,
                                         unsigned int size,
                                         unsigned int count,
                                         fi_handle handle)
    {
        return static_cast<unsigned int>(
            fwrite(buffer, size, count, reinterpret_cast<FILE*>(handle)));
    }

    static inline int seekProc(fi_handle handle, long offset, int origin)
    {
        return fseek(reinterpret_cast<FILE*>(handle), offset, origin);
    }

    static inline long int tellProc(fi_handle handle)
    {
        return ftell(reinterpret_cast<FILE*>(handle));
    }

    Renderer* mRenderer;
    glm::vec2 mTargetSize;
    std::shared_ptr<TextureResource> mTexture;
    std::vector<uint8_t> mPictureRGBA;
    size_t mFrameSize;

    std::chrono::time_point<std::chrono::system_clock> mAnimationStartTime;
    FILE* mAnimFile;
    FreeImageIO mAnimIO;
    FIMULTIBITMAP* mAnimation;
    FIBITMAP* mFrame;
    std::string mPath;
    std::string mStartDirection;
    std::string mDirection;
    int mTotalFrames;
    int mFrameNum;
    int mFrameTime;

    unsigned int mFileWidth;
    unsigned int mFileHeight;

    double mFrameRate;
    float mSpeedModifier;
    int mTargetPacing;
    int mTimeAccumulator;
    int mLastRenderedFrame;
    int mSkippedFrames;

    bool mHoldFrame;
    bool mPause;
    bool mExternalPause;
    bool mAlternate;
    int mIterationCount;
    int mPlayCount;
    bool mTargetIsMax;

    unsigned int mColorShift;
    unsigned int mColorShiftEnd;
    bool mColorGradientHorizontal;
};

#endif // ES_CORE_COMPONENTS_GIF_ANIM_COMPONENT_H
