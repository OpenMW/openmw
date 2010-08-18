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

namespace MWMechanics
{
    class MechanicsManager;
}

namespace MWDialogue
{
    class DialogueManager;
}

namespace MWWorld
{
    class World;

    ///< Collection of script-accessable sub-systems
    class Environment
    {
    public:
        Environment()
        : mWorld (0), mSoundManager (0), mGlobalScripts (0), mWindowManager (0),
          mMechanicsManager (0), mDialogueManager (0), mFrameDuration (0)
        {}

        World *mWorld;
        MWSound::SoundManager *mSoundManager;
        MWScript::GlobalScripts *mGlobalScripts;
        MWGui::WindowManager *mWindowManager;
        MWMechanics::MechanicsManager *mMechanicsManager;
        MWDialogue::DialogueManager *mDialogueManager;
        float mFrameDuration;
    };
}

#endif
