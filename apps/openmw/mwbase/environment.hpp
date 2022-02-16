#ifndef GAME_BASE_ENVIRONMENT_H
#define GAME_BASE_ENVIRONMENT_H

#include <memory>

namespace osg
{
    class Stats;
}

namespace Resource
{
    class ResourceSystem;
}

namespace MWBase
{
    class World;
    class ScriptManager;
    class DialogueManager;
    class Journal;
    class SoundManager;
    class MechanicsManager;
    class InputManager;
    class WindowManager;
    class StateManager;
    class LuaManager;

    /// \brief Central hub for mw-subsystems
    ///
    /// This class allows each mw-subsystem to access any others subsystem's top-level manager class.
    ///
    class Environment
    {
            static Environment *sThis;

            std::unique_ptr<World> mWorld;
            std::unique_ptr<SoundManager> mSoundManager;
            std::unique_ptr<ScriptManager> mScriptManager;
            std::unique_ptr<WindowManager> mWindowManager;
            std::unique_ptr<MechanicsManager> mMechanicsManager;
            std::unique_ptr<DialogueManager> mDialogueManager;
            std::unique_ptr<Journal> mJournal;
            std::unique_ptr<InputManager> mInputManager;
            std::unique_ptr<StateManager> mStateManager;
            std::unique_ptr<LuaManager> mLuaManager;
            Resource::ResourceSystem* mResourceSystem{};
            float mFrameDuration{};
            float mFrameRateLimit{};

            Environment (const Environment&);
            ///< not implemented

            Environment& operator= (const Environment&);
            ///< not implemented

        public:

            Environment();

            ~Environment();

            void setWorld (std::unique_ptr<World>&& world);

            void setSoundManager (std::unique_ptr<SoundManager>&& soundManager);

            void setScriptManager (std::unique_ptr<ScriptManager>&& scriptManager);

            void setWindowManager (std::unique_ptr<WindowManager>&& windowManager);

            void setMechanicsManager (std::unique_ptr<MechanicsManager>&& mechanicsManager);

            void setDialogueManager (std::unique_ptr<DialogueManager>&& dialogueManager);

            void setJournal (std::unique_ptr<Journal>&& journal);

            void setInputManager (std::unique_ptr<InputManager>&& inputManager);

            void setStateManager (std::unique_ptr<StateManager>&& stateManager);

            void setLuaManager (std::unique_ptr<LuaManager>&& luaManager);

            void setResourceSystem (Resource::ResourceSystem *resourceSystem);

            void setFrameDuration (float duration);
            ///< Set length of current frame in seconds.

            void setFrameRateLimit(float frameRateLimit);
            float getFrameRateLimit() const;

            World *getWorld() const;

            SoundManager *getSoundManager() const;

            ScriptManager *getScriptManager() const;

            WindowManager *getWindowManager() const;

            MechanicsManager *getMechanicsManager() const;

            DialogueManager *getDialogueManager() const;

            Journal *getJournal() const;

            InputManager *getInputManager() const;

            StateManager *getStateManager() const;

            LuaManager *getLuaManager() const;

            Resource::ResourceSystem *getResourceSystem() const;

            float getFrameDuration() const;

            void cleanup();
            ///< Delete all mw*-subsystems.

            static const Environment& get();
            ///< Return instance of this class.

            void reportStats(unsigned int frameNumber, osg::Stats& stats) const;
    };
}

#endif
