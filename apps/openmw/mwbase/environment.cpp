#include "environment.hpp"

#include <cassert>

#include <components/resource/resourcesystem.hpp>

#include "world.hpp"
#include "scriptmanager.hpp"
#include "dialoguemanager.hpp"
#include "journal.hpp"
#include "soundmanager.hpp"
#include "mechanicsmanager.hpp"
#include "inputmanager.hpp"
#include "windowmanager.hpp"
#include "statemanager.hpp"
#include "luamanager.hpp"

MWBase::Environment *MWBase::Environment::sThis = nullptr;

MWBase::Environment::Environment()
{
    assert(!sThis);
    sThis = this;
}

MWBase::Environment::~Environment()
{
    sThis = nullptr;
}

void MWBase::Environment::setWorld (std::unique_ptr<World>&& world)
{
    mWorld = std::move(world);
}

void MWBase::Environment::setSoundManager (std::unique_ptr<SoundManager>&& soundManager)
{
    mSoundManager = std::move(soundManager);
}

void MWBase::Environment::setScriptManager (std::unique_ptr<ScriptManager>&& scriptManager)
{
    mScriptManager = std::move(scriptManager);
}

void MWBase::Environment::setWindowManager (std::unique_ptr<WindowManager>&& windowManager)
{
    mWindowManager = std::move(windowManager);
}

void MWBase::Environment::setMechanicsManager (std::unique_ptr<MechanicsManager>&& mechanicsManager)
{
    mMechanicsManager = std::move(mechanicsManager);
}

void MWBase::Environment::setDialogueManager (std::unique_ptr<DialogueManager>&& dialogueManager)
{
    mDialogueManager = std::move(dialogueManager);
}

void MWBase::Environment::setJournal (std::unique_ptr<Journal>&& journal)
{
    mJournal = std::move(journal);
}

void MWBase::Environment::setInputManager (std::unique_ptr<InputManager>&& inputManager)
{
    mInputManager = std::move(inputManager);
}

void MWBase::Environment::setStateManager (std::unique_ptr<StateManager>&& stateManager)
{
    mStateManager = std::move(stateManager);
}

void MWBase::Environment::setLuaManager (std::unique_ptr<LuaManager>&& luaManager)
{
    mLuaManager = std::move(luaManager);
}

void MWBase::Environment::setResourceSystem (Resource::ResourceSystem *resourceSystem)
{
    mResourceSystem = resourceSystem;
}

void MWBase::Environment::setFrameDuration (float duration)
{
    mFrameDuration = duration;
}

void MWBase::Environment::setFrameRateLimit(float limit)
{
    mFrameRateLimit = limit;
}

float MWBase::Environment::getFrameRateLimit() const
{
    return mFrameRateLimit;
}

MWBase::World *MWBase::Environment::getWorld() const
{
    assert (mWorld);
    return mWorld.get();
}

MWBase::SoundManager *MWBase::Environment::getSoundManager() const
{
    assert (mSoundManager);
    return mSoundManager.get();
}

MWBase::ScriptManager *MWBase::Environment::getScriptManager() const
{
    assert (mScriptManager);
    return mScriptManager.get();
}

MWBase::WindowManager *MWBase::Environment::getWindowManager() const
{
    assert (mWindowManager);
    return mWindowManager.get();
}

MWBase::MechanicsManager *MWBase::Environment::getMechanicsManager() const
{
    assert (mMechanicsManager);
    return mMechanicsManager.get();
}

MWBase::DialogueManager *MWBase::Environment::getDialogueManager() const
{
    assert (mDialogueManager);
    return mDialogueManager.get();
}

MWBase::Journal *MWBase::Environment::getJournal() const
{
    assert (mJournal);
    return mJournal.get();
}

MWBase::InputManager *MWBase::Environment::getInputManager() const
{
    assert (mInputManager);
    return mInputManager.get();
}

MWBase::StateManager *MWBase::Environment::getStateManager() const
{
    assert (mStateManager);
    return mStateManager.get();
}

MWBase::LuaManager *MWBase::Environment::getLuaManager() const
{
    assert (mLuaManager);
    return mLuaManager.get();
}

Resource::ResourceSystem *MWBase::Environment::getResourceSystem() const
{
    return mResourceSystem;
}

float MWBase::Environment::getFrameDuration() const
{
    return mFrameDuration;
}

void MWBase::Environment::cleanup()
{
    mMechanicsManager.reset();
    mDialogueManager.reset();
    mJournal.reset();
    mScriptManager.reset();
    mWindowManager.reset();
    mWorld.reset();
    mSoundManager.reset();
    mInputManager.reset();
    mStateManager.reset();
    mLuaManager.reset();
    mResourceSystem = nullptr;
}

const MWBase::Environment& MWBase::Environment::get()
{
    assert (sThis);
    return *sThis;
}

void MWBase::Environment::reportStats(unsigned int frameNumber, osg::Stats& stats) const
{
    mMechanicsManager->reportStats(frameNumber, stats);
    mWorld->reportStats(frameNumber, stats);
}
