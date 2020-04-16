#include "keyboardmanager.hpp"

#include <MyGUI_InputManager.h>

#include <components/sdlutil/sdlinputwrapper.hpp>

#include <extern/oics/ICSInputControlSystem.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/statemanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/player.hpp"

#include "actionmanager.hpp"
#include "actions.hpp"

namespace MWInput
{
    KeyboardManager::KeyboardManager(ICS::InputControlSystem* inputBinder, SDLUtil::InputWrapper* inputWrapper, ActionManager* actionManager)
        : mInputBinder(inputBinder)
        , mInputWrapper(inputWrapper)
        , mActionManager(actionManager)
        , mControlsDisabled(false)
    {
    }

    bool KeyboardManager::actionIsActive (int id)
    {
        return (mInputBinder->getChannel(id)->getValue ()==1.0);
    }

    void KeyboardManager::textInput(const SDL_TextInputEvent &arg)
    {
        MyGUI::UString ustring(&arg.text[0]);
        MyGUI::UString::utf32string utf32string = ustring.asUTF32();
        for (MyGUI::UString::utf32string::const_iterator it = utf32string.begin(); it != utf32string.end(); ++it)
            MyGUI::InputManager::getInstance().injectKeyPress(MyGUI::KeyCode::None, *it);
    }

    void KeyboardManager::keyPressed(const SDL_KeyboardEvent &arg)
    {
        // HACK: to make Morrowind's default keybinding for the console work without printing an extra "^" upon closing
        // This assumes that SDL_TextInput events always come *after* the key event
        // (which is somewhat reasonable, and hopefully true for all SDL platforms)
        OIS::KeyCode kc = mInputWrapper->sdl2OISKeyCode(arg.keysym.sym);
        if (mInputBinder->getKeyBinding(mInputBinder->getControl(A_Console), ICS::Control::INCREASE)
                == arg.keysym.scancode
                && MWBase::Environment::get().getWindowManager()->isConsoleMode())
            SDL_StopTextInput();

        bool consumed = false;
        if (kc != OIS::KC_UNASSIGNED && !mInputBinder->detectingBindingState())
        {
            consumed = MWBase::Environment::get().getWindowManager()->injectKeyPress(MyGUI::KeyCode::Enum(kc), 0, arg.repeat);
            if (SDL_IsTextInputActive() &&  // Little trick to check if key is printable
                                    ( !(SDLK_SCANCODE_MASK & arg.keysym.sym) && std::isprint(arg.keysym.sym)))
                consumed = true;
            MWBase::Environment::get().getInputManager()->setPlayerControlsEnabled(!consumed);
        }
        if (arg.repeat)
            return;

        if (!mControlsDisabled && !consumed)
            mInputBinder->keyPressed(arg);
        MWBase::Environment::get().getInputManager()->setJoystickLastUsed(false);
    }

    void KeyboardManager::keyReleased(const SDL_KeyboardEvent &arg)
    {
        MWBase::Environment::get().getInputManager()->setJoystickLastUsed(false);
        OIS::KeyCode kc = mInputWrapper->sdl2OISKeyCode(arg.keysym.sym);

        if (!mInputBinder->detectingBindingState())
            MWBase::Environment::get().getInputManager()->setPlayerControlsEnabled(!MyGUI::InputManager::getInstance().injectKeyRelease(MyGUI::KeyCode::Enum(kc)));
        mInputBinder->keyReleased(arg);
    }
}
