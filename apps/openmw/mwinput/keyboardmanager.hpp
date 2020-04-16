#ifndef MWINPUT_MWKEYBOARDMANAGER_H
#define MWINPUT_MWKEYBOARDMANAGER_H

#include <components/settings/settings.hpp>
#include <components/sdlutil/events.hpp>

namespace SDLUtil
{
    class InputWrapper;
}

namespace ICS
{
    class InputControlSystem;
}

namespace MWInput
{
    class ActionManager;

    class KeyboardManager : public SDLUtil::KeyListener
    {
    public:
        KeyboardManager(ICS::InputControlSystem* inputBinder, SDLUtil::InputWrapper* inputWrapper, ActionManager* actionManager);

        virtual ~KeyboardManager() = default;

        void update(float dt, bool disableControls);

        virtual void textInput(const SDL_TextInputEvent &arg);
        virtual void keyPressed(const SDL_KeyboardEvent &arg);
        virtual void keyReleased(const SDL_KeyboardEvent &arg);

    private:
        bool actionIsActive(int id);

        ICS::InputControlSystem* mInputBinder;
        SDLUtil::InputWrapper* mInputWrapper;

        ActionManager* mActionManager;

        bool mControlsDisabled;
    };
}
#endif
