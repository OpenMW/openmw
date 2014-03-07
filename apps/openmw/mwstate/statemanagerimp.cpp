
#include "statemanagerimp.hpp"

#include <components/esm/esmwriter.hpp>
#include <components/esm/esmreader.hpp>
#include <components/esm/cellid.hpp>
#include <components/esm/loadcell.hpp>

#include <components/misc/stringops.hpp>

#include <components/settings/settings.hpp>

#include <OgreImage.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwbase/journal.hpp"
#include "../mwbase/dialoguemanager.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/mechanicsmanager.hpp"
#include "../mwbase/scriptmanager.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwworld/player.hpp"
#include "../mwworld/class.hpp"
#include "../mwworld/cellstore.hpp"

#include "../mwmechanics/npcstats.hpp"

#include "../mwscript/globalscripts.hpp"

void MWState::StateManager::cleanup (bool force)
{
    if (mState!=State_NoGame || force)
    {
        MWBase::Environment::get().getSoundManager()->clear();
        MWBase::Environment::get().getDialogueManager()->clear();
        MWBase::Environment::get().getJournal()->clear();
        MWBase::Environment::get().getScriptManager()->getGlobalScripts().clear();
        MWBase::Environment::get().getWorld()->clear();
        MWBase::Environment::get().getWindowManager()->clear();

        mState = State_NoGame;
        mCharacterManager.clearCurrentCharacter();
        mTimePlayed = 0;
    }
}

std::map<int, int> MWState::StateManager::buildContentFileIndexMap (const ESM::ESMReader& reader)
    const
{
    const std::vector<std::string>& current =
        MWBase::Environment::get().getWorld()->getContentFiles();

    const std::vector<ESM::Header::MasterData>& prev = reader.getGameFiles();

    std::map<int, int> map;

    for (int iPrev = 0; iPrev<static_cast<int> (prev.size()); ++iPrev)
    {
        std::string id = Misc::StringUtils::lowerCase (prev[iPrev].name);

        for (int iCurrent = 0; iCurrent<static_cast<int> (current.size()); ++iCurrent)
            if (id==Misc::StringUtils::lowerCase (current[iCurrent]))
            {
                map.insert (std::make_pair (iPrev, iCurrent));
                break;
            }
    }

    return map;
}

MWState::StateManager::StateManager (const boost::filesystem::path& saves, const std::string& game)
: mQuitRequest (false), mAskLoadRecent(false), mState (State_NoGame), mCharacterManager (saves, game), mTimePlayed (0)
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

void MWState::StateManager::askLoadRecent()
{
    if (MWBase::Environment::get().getWindowManager()->getMode() == MWGui::GM_MainMenu)
        return;

    if( !mAskLoadRecent )
    {
        if(getCurrentCharacter()->begin() == getCurrentCharacter()->end() )//no saves
        {
            MWBase::Environment::get().getWindowManager()->pushGuiMode (MWGui::GM_MainMenu);
        }
        else
        {
            MWState::Slot lastSave = *getCurrentCharacter()->begin();
            std::vector<std::string> buttons;
            buttons.push_back("#{sYes}");
            buttons.push_back("#{sNo}");
            std::string tag("%s");
            std::string message = MWBase::Environment::get().getWindowManager()->getGameSettingString("sLoadLastSaveMsg", tag);
            size_t pos = message.find(tag);
            message.replace(pos, tag.length(), lastSave.mProfile.mDescription);
            MWBase::Environment::get().getWindowManager()->messageBox(message, buttons);
            mAskLoadRecent = true;
        }
    }
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
    else
        MWBase::Environment::get().getWorld()->setGlobalInt ("chargenstate", -1);

    MWBase::Environment::get().getScriptManager()->getGlobalScripts().addStartup();

    mState = State_Running;
}

void MWState::StateManager::endGame()
{
    mState = State_Ended;
    MWBase::Environment::get().getWorld()->useDeathCamera();
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

    int screenshotW = 259*2, screenshotH = 133*2; // *2 to get some nice antialiasing
    Ogre::Image screenshot;
    world.screenshot(screenshot, screenshotW, screenshotH);
    Ogre::DataStreamPtr encoded = screenshot.encode("jpg");
    profile.mScreenshot.resize(encoded->size());
    encoded->read(&profile.mScreenshot[0], encoded->size());

    if (!slot)
        slot = mCharacterManager.getCurrentCharacter()->createSlot (profile);
    else
        slot = mCharacterManager.getCurrentCharacter()->updateSlot (slot, profile);

    std::ofstream stream (slot->mPath.string().c_str(), std::ios::binary);

    ESM::ESMWriter writer;

    const std::vector<std::string>& current =
        MWBase::Environment::get().getWorld()->getContentFiles();

    for (std::vector<std::string>::const_iterator iter (current.begin()); iter!=current.end();
        ++iter)
        writer.addMaster (*iter, 0); // not using the size information anyway -> use value of 0

    writer.setFormat (ESM::Header::CurrentFormat);
    writer.setRecordCount (
        1 // saved game header
        +MWBase::Environment::get().getJournal()->countSavedGameRecords()
        +MWBase::Environment::get().getWorld()->countSavedGameRecords()
        +MWBase::Environment::get().getScriptManager()->getGlobalScripts().countSavedGameRecords()
        +MWBase::Environment::get().getDialogueManager()->countSavedGameRecords()
        +1 // global map
        );

    writer.save (stream);

    writer.startRecord (ESM::REC_SAVE);
    slot->mProfile.save (writer);
    writer.endRecord (ESM::REC_SAVE);

    MWBase::Environment::get().getJournal()->write (writer);
    MWBase::Environment::get().getDialogueManager()->write (writer);
    MWBase::Environment::get().getWorld()->write (writer);
    MWBase::Environment::get().getScriptManager()->getGlobalScripts().write (writer);
    MWBase::Environment::get().getWindowManager()->write(writer);

    writer.close();

    Settings::Manager::setString ("character", "Saves",
        slot->mPath.parent_path().filename().string());
}

void MWState::StateManager::loadGame (const Character *character, const Slot *slot)
{
    try
    {
        cleanup();

        mTimePlayed = slot->mProfile.mTimePlayed;

        ESM::ESMReader reader;
        reader.open (slot->mPath.string());

        std::map<int, int> contentFileMap = buildContentFileIndexMap (reader);

        while (reader.hasMoreRecs())
        {
            ESM::NAME n = reader.getRecName();
            reader.getRecHeader();

            switch (n.val)
            {
                case ESM::REC_SAVE:

                    // don't need to read that here
                    reader.skipRecord();
                    break;

                case ESM::REC_JOUR:
                case ESM::REC_QUES:

                    MWBase::Environment::get().getJournal()->readRecord (reader, n.val);
                    break;

                case ESM::REC_DIAS:

                    MWBase::Environment::get().getDialogueManager()->readRecord (reader, n.val);
                    break;

                case ESM::REC_ALCH:
                case ESM::REC_ARMO:
                case ESM::REC_BOOK:
                case ESM::REC_CLAS:
                case ESM::REC_CLOT:
                case ESM::REC_ENCH:
                case ESM::REC_NPC_:
                case ESM::REC_SPEL:
                case ESM::REC_WEAP:
                case ESM::REC_GLOB:
                case ESM::REC_PLAY:
                case ESM::REC_CSTA:

                    MWBase::Environment::get().getWorld()->readRecord (reader, n.val, contentFileMap);
                    break;

                case ESM::REC_GSCR:

                    MWBase::Environment::get().getScriptManager()->getGlobalScripts().readRecord (reader, n.val);
                    break;

                case ESM::REC_GMAP:

                    MWBase::Environment::get().getWindowManager()->readRecord(reader, n.val);
                    break;

                default:

                    // ignore invalid records
                    /// \todo log error
                    reader.skipRecord();
            }
        }

        mCharacterManager.setCurrentCharacter(character);

        mState = State_Running;

        Settings::Manager::setString ("character", "Saves",
            slot->mPath.parent_path().filename().string());

        MWBase::Environment::get().getWindowManager()->setNewGame(false);
        MWBase::Environment::get().getWorld()->setupPlayer();
        MWBase::Environment::get().getWorld()->renderPlayer();
        MWBase::Environment::get().getWindowManager()->updatePlayer();
        MWBase::Environment::get().getMechanicsManager()->playerLoaded();

        MWWorld::Ptr ptr = MWBase::Environment::get().getWorld()->getPlayer().getPlayer();

        ESM::CellId cellId = ptr.getCell()->getCell()->getCellId();

        MWBase::Environment::get().getWorld()->changeToCell (cellId, ptr.getRefData().getPosition());
    }
    catch (const std::exception& e)
    {
        std::cerr << "failed to load saved game: " << e.what() << std::endl;
        cleanup (true);
    }
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

    // Note: It would be nicer to trigger this from InputManager, i.e. the very beginning of the frame update.
    if (mAskLoadRecent)
    {
        int iButton = MWBase::Environment::get().getWindowManager()->readPressedButton();
        if(iButton==0)
        {
            mAskLoadRecent = false;
            //Load last saved game for current character
            MWState::Character *curCharacter = getCurrentCharacter();
            MWState::Slot lastSave = *curCharacter->begin();
            loadGame(curCharacter, &lastSave);
        }
        else if(iButton==1)
        {
            mAskLoadRecent = false;
            MWBase::Environment::get().getWindowManager()->pushGuiMode (MWGui::GM_MainMenu);
        }
    }
}
