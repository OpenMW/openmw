#ifndef MWINPUT_MWMOUSEMANAGER_H
#define MWINPUT_MWMOUSEMANAGER_H

#include <components/settings/settings.hpp>
#include <components/sdlutil/events.hpp>

namespace SDLUtil
{
    class InputWrapper;
}

namespace MWInput
{
    class BindingsManager;

    class MouseManager : public SDLUtil::MouseListener
    {
    public:
        MouseManager(BindingsManager* bindingsManager, SDLUtil::InputWrapper* inputWrapper, SDL_Window* window);

        virtual ~MouseManager() = default;

        void update(float dt, bool disableControls);

        virtual void mouseMoved(const SDLUtil::MouseMotionEvent &arg);
        virtual void mousePressed(const SDL_MouseButtonEvent &arg, Uint8 id);
        virtual void mouseReleased(const SDL_MouseButtonEvent &arg, Uint8 id);
        virtual void mouseWheelMoved(const SDL_MouseWheelEvent &arg);

        void processChangedSettings(const Settings::CategorySettingVector& changed);

        bool injectMouseButtonPress(Uint8 button);
        bool injectMouseButtonRelease(Uint8 button);
        void injectMouseMove(int xMove, int yMove, int mouseWheelMove);
        void warpMouse();

        void setMouseLookEnabled(bool enabled) { mMouseLookEnabled = enabled; }
        void setGuiCursorEnabled(bool enabled) { mGuiCursorEnabled = enabled; }

    private:
        bool mInvertX;
        bool mInvertY;
        float mCameraSensitivity;
        float mCameraYMultiplier;

        BindingsManager* mBindingsManager;
        SDLUtil::InputWrapper* mInputWrapper;
        float mInvUiScalingFactor;

        float mGuiCursorX;
        float mGuiCursorY;
        int mMouseWheel;
        bool mMouseLookEnabled;
        bool mControlsDisabled;
        bool mGuiCursorEnabled;
    };
}
#endif
