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
    assert(sThis == nullptr);
    sThis = this;
}

MWBase::Environment::~Environment()
{
    sThis = nullptr;
}

void MWBase::Environment::cleanup()
{
    mMechanicsManager = nullptr;
    mDialogueManager = nullptr;
    mJournal = nullptr;
    mScriptManager = nullptr;
    mWindowManager = nullptr;
    mWorld = nullptr;
    mSoundManager = nullptr;
    mInputManager = nullptr;
    mStateManager = nullptr;
    mLuaManager = nullptr;
    mResourceSystem = nullptr;
}

void MWBase::Environment::reportStats(unsigned int frameNumber, osg::Stats& stats) const
{
    mMechanicsManager->reportStats(frameNumber, stats);
    mWorld->reportStats(frameNumber, stats);
}
