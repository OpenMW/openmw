
#include "statemanagerimp.hpp"

#include <components/esm/esmwriter.hpp>
#include <components/esm/esmreader.hpp>

#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"

#include "../mwmechanics/npcstats.hpp"

void MWState::StateManager::cleanup()
{
    if (mState!=State_NoGame)
    {
        MWBase::Environment::get().getDialogueManager()->clear();
        MWBase::Environment::get().getJournal()->clear();
        mState = State_NoGame;
        mCharacterManager.clearCurrentCharacter();
        mTimePlayed = 0;
    }
}

MWState::StateManager::StateManager (const boost::filesystem::path& saves, const std::string& game)
: mQuitRequest (false), mState (State_NoGame), mCharacterManager (saves, game), mTimePlayed (0)
{

}

void MWState::StateManager::requestQuit()
{
    mQuitRequest = true;
}

bool MWState::StateManager::hasQuitRequest() const
{
    return mQuitRequest;
}

MWState::StateManager::State MWState::StateManager::getState() const
{
    return mState;
}

void MWState::StateManager::newGame (bool bypass)
{
    cleanup();

    if (!bypass)
    {
        MWBase::Environment::get().getWorld()->startNewGame();
        MWBase::Environment::get().getWindowManager()->setNewGame (true);
    }

    mState = State_Running;
}

void MWState::StateManager::endGame()
{
    mState = State_Ended;
}

void MWState::StateManager::saveGame (const std::string& description, const Slot *slot)
{
    ESM::SavedGame profile;

    MWBase::World& world = *MWBase::Environment::get().getWorld();

    MWWorld::Ptr player = world.getPlayer().getPlayer();

    profile.mContentFiles = world.getContentFiles();

    profile.mPlayerName = player.getClass().getName (player);
    profile.mPlayerLevel = player.getClass().getNpcStats (player).getLevel();
    profile.mPlayerClass = player.get<ESM::NPC>()->mBase->mClass;

    profile.mPlayerCell = world.getCellName();

    profile.mInGameTime.mGameHour = world.getTimeStamp().getHour();
    profile.mInGameTime.mDay = world.getDay();
    profile.mInGameTime.mMonth = world.getMonth();
    profile.mInGameTime.mYear = world.getYear();
    profile.mTimePlayed = mTimePlayed;
    profile.mDescription = description;

    if (!slot)
        slot = mCharacterManager.getCurrentCharacter()->createSlot (profile);
    else
        slot = mCharacterManager.getCurrentCharacter()->updateSlot (slot, profile);

    std::ofstream stream (slot->mPath.string().c_str());
    ESM::ESMWriter writer;
    writer.setFormat (ESM::Header::CurrentFormat);
    writer.save (stream);
    writer.startRecord ("SAVE");
    slot->mProfile.save (writer);
    writer.endRecord ("SAVE");

    /// \todo write saved game data

    writer.close();

    Settings::Manager::setString ("character", "Saves",
        slot->mPath.parent_path().filename().string());
}

void MWState::StateManager::loadGame (const Character *character, const Slot *slot)
{
    cleanup();

    mTimePlayed = slot->mProfile.mTimePlayed;

    ESM::ESMReader reader;
    reader.open (slot->mPath.string());

    reader.getRecName(); // don't need to read that here
    reader.getRecHeader();

    /// \todo read saved game data

    mCharacterManager.setCurrentCharacter(character);

    mState = State_Running;

    Settings::Manager::setString ("character", "Saves",
        slot->mPath.parent_path().filename().string());
}

MWState::Character *MWState::StateManager::getCurrentCharacter (bool create)
{
    return mCharacterManager.getCurrentCharacter (create);
}

MWState::StateManager::CharacterIterator MWState::StateManager::characterBegin()
{
    return mCharacterManager.begin();
}

MWState::StateManager::CharacterIterator MWState::StateManager::characterEnd()
{
    return mCharacterManager.end();
}

void MWState::StateManager::update (float duration)
{
    mTimePlayed += duration;
}
