#include "keyboardmanager.hpp"

#include <cctype>

#include <MyGUI_InputManager.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/inputmanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/player.hpp"

#include "actions.hpp"
#include "bindingsmanager.hpp"
#include "sdlmappings.hpp"

namespace MWInput
{
    KeyboardManager::KeyboardManager(BindingsManager* bindingsManager)
        : mBindingsManager(bindingsManager)
        , mControlsDisabled(false)
    {
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
        // HACK: to make default keybinding for the console work without printing an extra "^" upon closing
        // This assumes that SDL_TextInput events always come *after* the key event
        // (which is somewhat reasonable, and hopefully true for all SDL platforms)
        auto kc = sdlKeyToMyGUI(arg.keysym.sym);
        if (mBindingsManager->getKeyBinding(A_Console) == arg.keysym.scancode
                && MWBase::Environment::get().getWindowManager()->isConsoleMode())
            SDL_StopTextInput();

        bool consumed = false;
        if (kc != MyGUI::KeyCode::None && !mBindingsManager->isDetectingBindingState())
        {
            consumed = MWBase::Environment::get().getWindowManager()->injectKeyPress(kc, 0, arg.repeat);
            if (SDL_IsTextInputActive() &&  // Little trick to check if key is printable
                                    (!(SDLK_SCANCODE_MASK & arg.keysym.sym) && std::isprint(arg.keysym.sym)))
                consumed = true;
            mBindingsManager->setPlayerControlsEnabled(!consumed);
        }

        if (arg.repeat)
            return;

        if (!mControlsDisabled && !consumed)
            mBindingsManager->keyPressed(arg);

        MWBase::Environment::get().getInputManager()->setJoystickLastUsed(false);
    }

    void KeyboardManager::keyReleased(const SDL_KeyboardEvent &arg)
    {
        MWBase::Environment::get().getInputManager()->setJoystickLastUsed(false);
        auto kc = sdlKeyToMyGUI(arg.keysym.sym);

        if (!mBindingsManager->isDetectingBindingState())
            mBindingsManager->setPlayerControlsEnabled(!MyGUI::InputManager::getInstance().injectKeyRelease(kc));
        mBindingsManager->keyReleased(arg);
    }
}
