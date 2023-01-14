//  SPDX-License-Identifier: MIT
//
//  EmulationStation Desktop Edition
//  PrimaryComponent.h
//
//  Base class for the primary components (carousel, grid and textlist).
//

#ifndef ES_CORE_COMPONENTS_PRIMARY_PRIMARY_COMPONENT_H
#define ES_CORE_COMPONENTS_PRIMARY_PRIMARY_COMPONENT_H

template <typename T> class PrimaryComponent : public virtual GuiComponent
{
public:
    enum class PrimaryType {
        CAROUSEL,
        GRID,
        TEXTLIST
    };

    enum class PrimaryAlignment {
        ALIGN_LEFT,
        ALIGN_CENTER,
        ALIGN_RIGHT
    };

    // IList functions.
    virtual bool isScrolling() const = 0;
    virtual void stopScrolling() = 0;
    virtual const int getScrollingVelocity() = 0;
    virtual void clear() = 0;
    virtual const T& getSelected() const = 0;
    virtual const T& getNext() const = 0;
    virtual const T& getPrevious() const = 0;
    virtual const T& getFirst() const = 0;
    virtual const T& getLast() const = 0;
    virtual bool setCursor(const T& obj) = 0;
    virtual bool remove(const T& obj) = 0;
    virtual int size() const = 0;

    // Functions used by all primary components.
    virtual void setCancelTransitionsCallback(const std::function<void()>& func) = 0;
    virtual void setCursorChangedCallback(const std::function<void(CursorState state)>& func) = 0;
    virtual int getCursor() = 0;
    virtual const size_t getNumEntries() = 0;
    virtual const bool getFadeAbovePrimary() const = 0;
    virtual const LetterCase getLetterCase() const = 0;
    virtual const LetterCase getLetterCaseAutoCollections() const = 0;
    virtual const LetterCase getLetterCaseCustomCollections() const = 0;
    virtual const bool getSystemNameSuffix() const = 0;
    virtual const LetterCase getLetterCaseSystemNameSuffix() const = 0;

    // Functions used by some primary components.
    virtual void onDemandTextureLoad() {}
    virtual void setAlignment(PrimaryAlignment align) {}
};

#endif // ES_CORE_COMPONENTS_PRIMARY_PRIMARY_COMPONENT_H
