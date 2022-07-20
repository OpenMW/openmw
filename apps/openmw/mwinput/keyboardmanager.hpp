#ifndef MWINPUT_MWKEYBOARDMANAGER_H
#define MWINPUT_MWKEYBOARDMANAGER_H

#include <components/sdlutil/events.hpp>

namespace MWInput
{
    class BindingsManager;

    class KeyboardManager : public SDLUtil::KeyListener
    {
    public:
        KeyboardManager(BindingsManager* bindingsManager);

        virtual ~KeyboardManager() = default;

        void textInput(const SDL_TextInputEvent &arg) override;
        void keyPressed(const SDL_KeyboardEvent &arg) override;
        void keyReleased(const SDL_KeyboardEvent &arg) override;

    private:
        BindingsManager* mBindingsManager;
    };
}
#endif
