#include "environment.hpp"

#include <cassert>
#include <chrono>
#include <thread>

#include "world.hpp"
#include "scriptmanager.hpp"
#include "dialoguemanager.hpp"
#include "journal.hpp"
#include "soundmanager.hpp"
#include "mechanicsmanager.hpp"
#include "inputmanager.hpp"
#include "windowmanager.hpp"
#include "statemanager.hpp"

MWBase::Environment *MWBase::Environment::sThis = 0;

MWBase::Environment::Environment()
: mWorld (0), mSoundManager (0), mScriptManager (0), mWindowManager (0),
  mMechanicsManager (0),  mDialogueManager (0), mJournal (0), mInputManager (0), mStateManager (0),
  mFrameDuration (0), mFrameRateLimit(0.f)
{
    assert (!sThis);
    sThis = this;
}

MWBase::Environment::~Environment()
{
    cleanup();
    sThis = 0;
}

void MWBase::Environment::setWorld (World *world)
{
    mWorld = world;
}

void MWBase::Environment::setSoundManager (SoundManager *soundManager)
{
    mSoundManager = soundManager;
}

void MWBase::Environment::setScriptManager (ScriptManager *scriptManager)
{
    mScriptManager = scriptManager;
}

void MWBase::Environment::setWindowManager (WindowManager *windowManager)
{
    mWindowManager = windowManager;
}

void MWBase::Environment::setMechanicsManager (MechanicsManager *mechanicsManager)
{
    mMechanicsManager = mechanicsManager;
}

void MWBase::Environment::setDialogueManager (DialogueManager *dialogueManager)
{
    mDialogueManager = dialogueManager;
}

void MWBase::Environment::setJournal (Journal *journal)
{
    mJournal = journal;
}

void MWBase::Environment::setInputManager (InputManager *inputManager)
{
    mInputManager = inputManager;
}

void MWBase::Environment::setStateManager (StateManager *stateManager)
{
    mStateManager = stateManager;
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

void MWBase::Environment::limitFrameRate(double dt) const
{
    if (mFrameRateLimit > 0.f)
    {
        double thisFrameTime = dt;
        double minFrameTime = 1.0 / static_cast<double>(mFrameRateLimit);
        if (thisFrameTime < minFrameTime)
        {
            std::this_thread::sleep_for(std::chrono::duration<double>(minFrameTime - thisFrameTime));
        }
    }
}

MWBase::World *MWBase::Environment::getWorld() const
{
    assert (mWorld);
    return mWorld;
}

MWBase::SoundManager *MWBase::Environment::getSoundManager() const
{
    assert (mSoundManager);
    return mSoundManager;
}

MWBase::ScriptManager *MWBase::Environment::getScriptManager() const
{
    assert (mScriptManager);
    return mScriptManager;
}

MWBase::WindowManager *MWBase::Environment::getWindowManager() const
{
    assert (mWindowManager);
    return mWindowManager;
}

MWBase::MechanicsManager *MWBase::Environment::getMechanicsManager() const
{
    assert (mMechanicsManager);
    return mMechanicsManager;
}

MWBase::DialogueManager *MWBase::Environment::getDialogueManager() const
{
    assert (mDialogueManager);
    return mDialogueManager;
}

MWBase::Journal *MWBase::Environment::getJournal() const
{
    assert (mJournal);
    return mJournal;
}

MWBase::InputManager *MWBase::Environment::getInputManager() const
{
    assert (mInputManager);
    return mInputManager;
}

MWBase::StateManager *MWBase::Environment::getStateManager() const
{
    assert (mStateManager);
    return mStateManager;
}

float MWBase::Environment::getFrameDuration() const
{
    return mFrameDuration;
}

void MWBase::Environment::cleanup()
{
    delete mMechanicsManager;
    mMechanicsManager = 0;

    delete mDialogueManager;
    mDialogueManager = 0;

    delete mJournal;
    mJournal = 0;

    delete mScriptManager;
    mScriptManager = 0;

    delete mWindowManager;
    mWindowManager = 0;

    delete mWorld;
    mWorld = 0;

    delete mSoundManager;
    mSoundManager = 0;

    delete mInputManager;
    mInputManager = 0;

    delete mStateManager;
    mStateManager = 0;
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
