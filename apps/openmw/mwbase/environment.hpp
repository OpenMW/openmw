#ifndef GAME_BASE_ENVIRONMENT_H
#define GAME_BASE_ENVIRONMENT_H

#include <components/misc/notnullptr.hpp>

#include <memory>

namespace Resource
{
    class ResourceSystem;
}

namespace L10n
{
    class Manager;
}

namespace MWWorld
{
    class ESMStore;
    class WorldModel;
    class Scene;
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
        static Environment* sThis;

        World* mWorld = nullptr;
        MWWorld::WorldModel* mWorldModel = nullptr;
        MWWorld::Scene* mWorldScene = nullptr;
        MWWorld::ESMStore* mESMStore = nullptr;
        SoundManager* mSoundManager = nullptr;
        ScriptManager* mScriptManager = nullptr;
        WindowManager* mWindowManager = nullptr;
        MechanicsManager* mMechanicsManager = nullptr;
        DialogueManager* mDialogueManager = nullptr;
        Journal* mJournal = nullptr;
        InputManager* mInputManager = nullptr;
        StateManager* mStateManager = nullptr;
        LuaManager* mLuaManager = nullptr;
        Resource::ResourceSystem* mResourceSystem = nullptr;
        L10n::Manager* mL10nManager = nullptr;
        float mFrameRateLimit = 0;
        float mFrameDuration = 0;

    public:
        Environment();

        ~Environment();

        Environment(const Environment&) = delete;

        Environment& operator=(const Environment&) = delete;

        void setWorld(World& value) { mWorld = &value; }
        void setWorldModel(MWWorld::WorldModel& value) { mWorldModel = &value; }
        void setWorldScene(MWWorld::Scene& value) { mWorldScene = &value; }
        void setESMStore(MWWorld::ESMStore& value) { mESMStore = &value; }

        void setSoundManager(SoundManager& value) { mSoundManager = &value; }

        void setScriptManager(ScriptManager& value) { mScriptManager = &value; }

        void setWindowManager(WindowManager& value) { mWindowManager = &value; }

        void setMechanicsManager(MechanicsManager& value) { mMechanicsManager = &value; }

        void setDialogueManager(DialogueManager& value) { mDialogueManager = &value; }

        void setJournal(Journal& value) { mJournal = &value; }

        void setInputManager(InputManager& value) { mInputManager = &value; }

        void setStateManager(StateManager& value) { mStateManager = &value; }

        void setLuaManager(LuaManager& value) { mLuaManager = &value; }

        void setResourceSystem(Resource::ResourceSystem& value) { mResourceSystem = &value; }

        void setL10nManager(L10n::Manager& value) { mL10nManager = &value; }

        Misc::NotNullPtr<World> getWorld() const { return mWorld; }
        Misc::NotNullPtr<MWWorld::WorldModel> getWorldModel() const { return mWorldModel; }
        Misc::NotNullPtr<MWWorld::Scene> getWorldScene() const { return mWorldScene; }
        Misc::NotNullPtr<MWWorld::ESMStore> getESMStore() const { return mESMStore; }

        Misc::NotNullPtr<SoundManager> getSoundManager() const { return mSoundManager; }

        Misc::NotNullPtr<ScriptManager> getScriptManager() const { return mScriptManager; }

        Misc::NotNullPtr<WindowManager> getWindowManager() const { return mWindowManager; }

        Misc::NotNullPtr<MechanicsManager> getMechanicsManager() const { return mMechanicsManager; }

        Misc::NotNullPtr<DialogueManager> getDialogueManager() const { return mDialogueManager; }

        Misc::NotNullPtr<Journal> getJournal() const { return mJournal; }

        Misc::NotNullPtr<InputManager> getInputManager() const { return mInputManager; }

        Misc::NotNullPtr<StateManager> getStateManager() const { return mStateManager; }

        Misc::NotNullPtr<LuaManager> getLuaManager() const { return mLuaManager; }

        Misc::NotNullPtr<Resource::ResourceSystem> getResourceSystem() const { return mResourceSystem; }

        Misc::NotNullPtr<L10n::Manager> getL10nManager() const { return mL10nManager; }

        float getFrameRateLimit() const { return mFrameRateLimit; }

        void setFrameRateLimit(float value) { mFrameRateLimit = value; }

        float getFrameDuration() const { return mFrameDuration; }

        void setFrameDuration(float value) { mFrameDuration = value; }

        /// Return instance of this class.
        static const Environment& get()
        {
            assert(sThis != nullptr);
            return *sThis;
        }
    };
}

#endif
