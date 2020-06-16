#ifndef MWINPUT_MWKEYBOARDMANAGER_H
#define MWINPUT_MWKEYBOARDMANAGER_H

#include <components/settings/settings.hpp>
#include <components/sdlutil/events.hpp>

namespace MWInput
{
    class BindingsManager;

    class KeyboardManager : public SDLUtil::KeyListener
    {
    public:
        KeyboardManager(BindingsManager* bindingsManager);

        virtual ~KeyboardManager() = default;

        virtual void textInput(const SDL_TextInputEvent &arg);
        virtual void keyPressed(const SDL_KeyboardEvent &arg);
        virtual void keyReleased(const SDL_KeyboardEvent &arg);

    private:
        BindingsManager* mBindingsManager;
    };
}
#endif
