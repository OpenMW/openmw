#ifndef GAME_BASE_ENVIRONMENT_H
#define GAME_BASE_ENVIRONMENT_H

#include <components/misc/notnullptr.hpp>

#include <functional>
#include <memory>
#include <optional>
#include <stdexcept>

namespace Resource
{
    class ResourceSystem;
}

namespace L10n
{
    class Manager;
}

namespace MWNet
{
    class NetworkManager;
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
    class LuaEventRouter;

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
        std::optional<std::reference_wrapper<LuaManager>> mLuaManager;
        std::optional<std::reference_wrapper<LuaManager>> mAuthoritativeLuaManager;
        std::optional<std::reference_wrapper<LuaManager>> mClientLuaManager;
        LuaEventRouter* mLuaEventRouter = nullptr;
        Resource::ResourceSystem* mResourceSystem = nullptr;
        L10n::Manager* mL10nManager = nullptr;
        MWNet::NetworkManager* mNetworkManager = nullptr;
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

        // Transitional default accessor for systems that still operate on the only current Lua runtime.
        void setLuaManager(LuaManager& value) { mLuaManager = std::ref(value); }

        void setAuthoritativeLuaManager(LuaManager& value) { mAuthoritativeLuaManager = std::ref(value); }

        void setClientLuaManager(LuaManager& value) { mClientLuaManager = std::ref(value); }

        void setLuaEventRouter(LuaEventRouter& value) { mLuaEventRouter = &value; }

        void clearLuaEventRouter() { mLuaEventRouter = nullptr; }

        void setResourceSystem(Resource::ResourceSystem& value) { mResourceSystem = &value; }

        void setL10nManager(L10n::Manager& value) { mL10nManager = &value; }

        void setNetworkManager(MWNet::NetworkManager& value) { mNetworkManager = &value; }

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

        bool hasAuthoritativeLuaManager() const { return mAuthoritativeLuaManager.has_value(); }

        bool hasClientLuaManager() const { return mClientLuaManager.has_value(); }

        // Transitional default accessor for systems that still operate on the only current Lua runtime.
        Misc::NotNullPtr<LuaManager> getLuaManager() const { return mLuaManager ? &mLuaManager->get() : nullptr; }

        Misc::NotNullPtr<LuaManager> getAuthoritativeLuaManager() const
        {
            if (!mAuthoritativeLuaManager)
                throw std::logic_error("Authoritative Lua manager is not registered");
            return &mAuthoritativeLuaManager->get();
        }

        Misc::NotNullPtr<LuaManager> getClientLuaManager() const
        {
            if (!mClientLuaManager)
                throw std::logic_error("Client Lua manager is not registered");
            return &mClientLuaManager->get();
        }

        Misc::NotNullPtr<LuaManager> getLuaManagerForGlobalScripts() const;

        void forEachLuaManagerAuthoritativeFirst(const std::function<void(LuaManager&)>& callback) const;

        void reloadAllLuaManagersAuthoritativeFirst() const;

        Misc::NotNullPtr<LuaEventRouter> getLuaEventRouter() const
        {
            if (mLuaEventRouter == nullptr)
                throw std::logic_error("Lua event router is not registered");
            return mLuaEventRouter;
        }

        Misc::NotNullPtr<Resource::ResourceSystem> getResourceSystem() const { return mResourceSystem; }

        Misc::NotNullPtr<L10n::Manager> getL10nManager() const { return mL10nManager; }

        Misc::NotNullPtr<MWNet::NetworkManager> getNetworkManager() const { return mNetworkManager; }

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
