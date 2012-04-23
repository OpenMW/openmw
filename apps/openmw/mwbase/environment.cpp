
#include "environment.hpp"

#include <cassert>

MWBase::Environment *MWBase::Environment::sThis = 0;

MWBase::Environment::Environment()
: mWorld (0), mSoundManager (0), mGlobalScripts (0), mScriptManager (0), mWindowManager (0),
  mMechanicsManager (0),  mDialogueManager (0), mJournal (0), mFrameDuration (0)
{
    assert (!sThis);
    sThis = this;
}

MWBase::Environment::~Environment()
{
    sThis = 0;
}

void MWBase::Environment::setWorld (MWWorld::World *world)
{
    mWorld = world;
}

void MWBase::Environment::setSoundManager (MWSound::SoundManager *soundManager)
{
    mSoundManager = soundManager;
}

void MWBase::Environment::setGlobalScripts (MWScript::GlobalScripts *globalScripts)
{
    mGlobalScripts = globalScripts;
}

void MWBase::Environment::setScriptManager (MWScript::ScriptManager *scriptManager)
{
    mScriptManager = scriptManager;
}

void MWBase::Environment::setWindowManager (MWGui::WindowManager *windowManager)
{
    mWindowManager = windowManager;
}

void MWBase::Environment::setMechanicsManager (MWMechanics::MechanicsManager *mechanicsManager)
{
    mMechanicsManager = mechanicsManager;
}

void MWBase::Environment::setDialogueManager (MWDialogue::DialogueManager *dialogueManager)
{
    mDialogueManager = dialogueManager;
}

void MWBase::Environment::setJournal (MWDialogue::Journal *journal)
{
    mJournal = journal;
}

void MWBase::Environment::setFrameDuration (float duration)
{
    mFrameDuration = duration;
}

MWWorld::World *MWBase::Environment::getWorld() const
{
    assert (mWorld);
    return mWorld;
}

MWSound::SoundManager *MWBase::Environment::getSoundManager() const
{
    assert (mSoundManager);
    return mSoundManager;
}

MWScript::GlobalScripts *MWBase::Environment::getGlobalScripts() const
{
    assert (mGlobalScripts);
    return mGlobalScripts;
}

MWScript::ScriptManager *MWBase::Environment::getScriptManager() const
{
    assert (mScriptManager);
    return mScriptManager;
}

MWGui::WindowManager *MWBase::Environment::getWindowManager() const
{
    assert (mWindowManager);
    return mWindowManager;
}

MWMechanics::MechanicsManager *MWBase::Environment::getMechanicsManager() const
{
    assert (mMechanicsManager);
    return mMechanicsManager;
}

MWDialogue::DialogueManager *MWBase::Environment::getDialogueManager() const
{
    assert (mDialogueManager);
    return mDialogueManager;
}

MWDialogue::Journal *MWBase::Environment::getJournal() const
{
    assert (mJournal);
    return mJournal;
}

float MWBase::Environment::getFrameDuration() const
{
    return mFrameDuration;
}

const MWBase::Environment& MWBase::Environment::get()
{
    assert (sThis);
    return *sThis;
}
