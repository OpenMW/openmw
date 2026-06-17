#ifndef MWGUI_WINDOW_BASE_H
#define MWGUI_WINDOW_BASE_H

#include <SDL_events.h>

#include "layout.hpp"

namespace MWWorld
{
    class Ptr;
}

namespace MWGui
{
    class DragAndDrop;

    size_t wrap(size_t index, size_t max, int delta);
    void setControllerFocus(const std::vector<MyGUI::Button*>& buttons, size_t index, bool selected);

    struct ControllerButtons
    {
        std::string mA;
        std::string mB;
        std::string mDpad;
        std::string mL1;
        std::string mL2;
        std::string mL3;
        std::string mLStick;
        std::string mMenu;
        std::string mR1;
        std::string mR2;
        std::string mR3;
        std::string mRStick;
        std::string mView;
        std::string mX;
        std::string mY;
    };

    class WindowBase : public Layout
    {
    public:
        WindowBase(std::string_view parLayout);

        virtual MyGUI::Widget* getDefaultKeyFocus() { return nullptr; }

        // Events
        typedef MyGUI::delegates::MultiDelegate<WindowBase*> EventHandle_WindowBase;

        /// Open this object in the GUI, for windows that support it
        virtual void setPtr(const MWWorld::Ptr& ptr) {}

        /// Called every frame if the window is in an active GUI mode
        virtual void onFrame(float duration) {}

        /// Notify that window has been made visible
        virtual void onOpen() {}
        /// Notify that window has been hidden
        virtual void onClose() {}
        /// Gracefully exits the window
        virtual bool exit() { return true; }
        /// Sets the visibility of the window
        void setVisible(bool visible) override;
        /// Returns the visibility state of the window
        bool isVisible() const;

        void center();

        /// Clear any state specific to the running game
        virtual void clear() {}

        /// Called when GUI viewport changes size
        virtual void onResChange(int width, int height) {}

        virtual void onDeleteCustomData(const MWWorld::Ptr& ptr) {}

        virtual void onInventoryUpdate(const MWWorld::Ptr& ptr) {}

        virtual std::string_view getWindowIdForLua() const { return {}; }
        void setDisabledByLua(bool disabled) { mDisabledByLua = disabled; }

        static void clampWindowCoordinates(MyGUI::Window* window);

        virtual ControllerButtons* getControllerButtons() { return &mControllerButtons; }
        MyGUI::Widget* getControllerScrollWidget() { return mControllerScrollWidget; }
        bool isGamepadCursorAllowed() { return !mDisableGamepadCursor; }
        virtual bool onControllerButtonEvent(const SDL_ControllerButtonEvent& arg) { return true; }
        virtual bool onControllerThumbstickEvent(const SDL_ControllerAxisEvent& arg) { return false; }
        virtual void setActiveControllerWindow(bool active) { mActiveControllerWindow = active; }

    protected:
        virtual void onTitleDoubleClicked();

        ControllerButtons mControllerButtons;
        bool mActiveControllerWindow = false;
        bool mDisableGamepadCursor = false;
        MyGUI::Widget* mControllerScrollWidget = nullptr;

    private:
        void onDoubleClick(MyGUI::Widget* sender);

        bool mDisabledByLua = false;
    };

    /*
     * "Modal" windows cause the rest of the interface to be inaccessible while they are visible
     */
    class WindowModal : public WindowBase
    {
    public:
        WindowModal(const std::string& parLayout);
        void onOpen() override;
        void onClose() override;
        bool exit() override { return true; }
    };

    /// A window that cannot be the target of a drag&drop action.
    /// When hovered with a drag item, the window will become transparent and allow click-through.
    class NoDrop
    {
    public:
        NoDrop(DragAndDrop* drag, MyGUI::Widget* widget);

        void onFrame(float dt);
        virtual void setAlpha(float alpha);
        virtual ~NoDrop() = default;

    private:
        MyGUI::Widget* mWidget;
        DragAndDrop* mDrag;
        bool mTransparent;
    };

    class BookWindowBase : public WindowBase
    {
    public:
        BookWindowBase(std::string_view parLayout);

    protected:
        float adjustButton(std::string_view name);
    };
}

#endif
