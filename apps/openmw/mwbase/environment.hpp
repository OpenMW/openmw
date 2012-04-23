#ifndef GAME_BASE_INVIRONMENT_H
#define GAME_BASE_INVIRONMENT_H

namespace MWSound
{
    class SoundManager;
}

namespace MWScript
{
    class ScriptManager;
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
    class Journal;
}

namespace MWInput
{
    struct MWInputManager;
}

namespace MWWorld
{
    class World;
}

namespace MWBase
{
    /// \brief Central hub for mw-subsystems
    ///
    /// This class allows each mw-subsystem to access any others subsystem's top-level manager class.
    ///
    /// \attention Environment does not take ownership of the manager class instances it is handed over in
    /// the set* functions.
    class Environment
    {
            static Environment *sThis;

            MWWorld::World *mWorld;
            MWSound::SoundManager *mSoundManager;
            MWScript::ScriptManager *mScriptManager;
            MWGui::WindowManager *mWindowManager;
            MWMechanics::MechanicsManager *mMechanicsManager;
            MWDialogue::DialogueManager *mDialogueManager;
            MWDialogue::Journal *mJournal;
            float mFrameDuration;

            Environment (const Environment&);
            ///< not implemented

            Environment& operator= (const Environment&);
            ///< not implemented

        public:

            Environment();

            ~Environment();

            void setWorld (MWWorld::World *world);

            void setSoundManager (MWSound::SoundManager *soundManager);

            void setScriptManager (MWScript::ScriptManager *scriptManager);

            void setWindowManager (MWGui::WindowManager *windowManager);

            void setMechanicsManager (MWMechanics::MechanicsManager *mechanicsManager);

            void setDialogueManager (MWDialogue::DialogueManager *dialogueManager);

            void setJournal (MWDialogue::Journal *journal);

            void setFrameDuration (float duration);
            ///< Set length of current frame in seconds.

            MWWorld::World *getWorld() const;

            MWSound::SoundManager *getSoundManager() const;

            MWScript::ScriptManager *getScriptManager() const;

            MWGui::WindowManager *getWindowManager() const;

            MWMechanics::MechanicsManager *getMechanicsManager() const;

            MWDialogue::DialogueManager *getDialogueManager() const;

            MWDialogue::Journal *getJournal() const;

            float getFrameDuration() const;

            static const Environment& get();
            ///< Return instance of this class.
    };
}

#endif
