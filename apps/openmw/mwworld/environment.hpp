#ifndef GAME_WORLD_INVIRONMENT_H
#define GAME_WORLD_INVIRONMENT_H

namespace MWSound
{
    class SoundManager;
}

namespace MWScript
{
    class GlobalScripts;
}

namespace MWGui
{
    class WindowManager;
}

namespace MWWorld
{
    class World;

    ///< Collection of script-accessable sub-systems
    class Environment
    {   
    public:
        Environment()
        : mWorld (0), mSoundManager (0), mGlobalScripts (0), mWindowManager (0), mFrameDuration (0)
        {}

        World *mWorld;
        MWSound::SoundManager *mSoundManager;
        MWScript::GlobalScripts *mGlobalScripts;
        MWGui::WindowManager *mWindowManager;
        float mFrameDuration;
    };
}

#endif

