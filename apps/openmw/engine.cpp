#include "engine.hpp"
#include "components/esm/loadcell.hpp"

#include <cassert>

#include <iostream>
#include <utility>

#include <OgreVector3.h>
#include <Ogre.h>

#include "components/esm/records.hpp"
#include <components/esm_store/cell_store.hpp>
#include <components/files/fileops.hpp>
#include <components/bsa/bsa_archive.hpp>
#include <components/esm/loadregn.hpp>
#include <components/esm/esm_reader.hpp>
#include <components/files/path.hpp>

#include <openengine/gui/manager.hpp>
#include "mwgui/window_manager.hpp"

#include "mwinput/inputmanager.hpp"

#include "mwscript/scriptmanager.hpp"
#include "mwscript/compilercontext.hpp"
#include "mwscript/interpretercontext.hpp"
#include "mwscript/extensions.hpp"
#include "mwscript/globalscripts.hpp"

#include "mwsound/soundmanager.hpp"

#include "mwworld/world.hpp"
#include "mwworld/ptr.hpp"
#include "mwworld/environment.hpp"
#include "mwworld/class.hpp"
#include "mwworld/player.hpp"

#include "mwclass/classes.hpp"

#include "mwdialogue/dialoguemanager.hpp"
#include "mwdialogue/journal.hpp"

#include "mwmechanics/mechanicsmanager.hpp"

#include <OgreRoot.h>

#include <MyGUI_WidgetManager.h>
#include "mwgui/class.hpp"

#include "components/nifbullet/bullet_nif_loader.hpp"

//using namespace ESM;

void OMW::Engine::executeLocalScripts()
{
    MWWorld::LocalScripts& localScripts = mEnvironment.mWorld->getLocalScripts();

    localScripts.startIteration();

    while (!localScripts.isFinished())
    {
        std::pair<std::string, MWWorld::Ptr> script = localScripts.getNext();

        MWScript::InterpreterContext interpreterContext (mEnvironment,
            &script.second.getRefData().getLocals(), script.second);
        mScriptManager->run (script.first, interpreterContext);

        if (mEnvironment.mWorld->hasCellChanged())
            break;
    }

    mEnvironment.mWorld->getLocalScripts().setIgnore (MWWorld::Ptr());
}

bool OMW::Engine::frameRenderingQueued (const Ogre::FrameEvent& evt)
{
    try
    {
        if(mShowFPS)
        {
            mEnvironment.mWindowManager->wmSetFPS(mOgre.getFPS());
        }

        if(mUseSound && !(mEnvironment.mSoundManager->isMusicPlaying()))
        {
            // Play some good 'ol tunes
            mEnvironment.mSoundManager->startRandomTitle();
        }

        std::string effect;

        MWWorld::Ptr::CellStore *current = mEnvironment.mWorld->getPlayer().getPlayer().getCell();

        //If the region has changed
        if(!(current->cell->data.flags & current->cell->Interior) && timer.elapsed() >= 10){
            timer.restart();
            if (test.name != current->cell->region)
            {
                total = 0;
                test = (ESM::Region) *(mEnvironment.mWorld->getStore().regions.find(current->cell->region));
            }

            if(test.soundList.size() > 0)
            {
                std::vector<ESM::Region::SoundRef>::iterator soundIter = test.soundList.begin();
                //mEnvironment.mSoundManager
                if(total == 0){
                    while (!(soundIter == test.soundList.end()))
                    {
                        ESM::NAME32 go = soundIter->sound;
                        int chance = (int) soundIter->chance;
                        //std::cout << "Sound: " << go.name <<" Chance:" <<  chance << "\n";
                        soundIter++;
                        total += chance;
                    }
                }

                int r = rand() % total;        //old random code
                int pos = 0;
                soundIter = test.soundList.begin();
                while (!(soundIter == test.soundList.end()))
                {
                    const ESM::NAME32 go = soundIter->sound;
                    int chance = (int) soundIter->chance;
                    //std::cout << "Sound: " << go.name <<" Chance:" <<  chance << "\n";
                    soundIter++;
                    if( r - pos < chance)
                    {
                        effect = go.name;
                        //play sound
                        std::cout << "Sound: " << go.name <<" Chance:" <<  chance << "\n";
                        mEnvironment.mSoundManager->playSound(effect, 20.0, 1.0);

                        break;

                    }
                    pos += chance;
                }
            }

            //mEnvironment.mSoundManager->playSound(effect, 1.0, 1.0);
            //printf("REGION: %s\n", test.name);

        }
        else if(current->cell->data.flags & current->cell->Interior)
        {
            test.name = "";
        }

        mEnvironment.mFrameDuration = evt.timeSinceLastFrame;

        //
        mEnvironment.mWindowManager->onFrame(mEnvironment.mFrameDuration);

        // global scripts
        mEnvironment.mGlobalScripts->run (mEnvironment);

        bool changed = mEnvironment.mWorld->hasCellChanged();

        // local scripts
        executeLocalScripts(); // This does not handle the case where a global script causes a cell
                               // change, followed by a cell change in a local script during the same
                               // frame.

        // passing of time
        if (mEnvironment.mWindowManager->getMode()==MWGui::GM_Game)
            mEnvironment.mWorld->advanceTime (
                mEnvironment.mFrameDuration*mEnvironment.mWorld->getTimeScaleFactor()/3600);

        if (changed) // keep change flag for another frame, if cell changed happend in local script
            mEnvironment.mWorld->markCellAsUnchanged();

        // update actors
        std::vector<std::pair<std::string, Ogre::Vector3> > movement;
        mEnvironment.mMechanicsManager->update (movement);

        if (focusFrameCounter++ == focusUpdateFrame)
        {
            std::string handle = mEnvironment.mWorld->getFacedHandle();

            std::string name;

            if (!handle.empty())
            {
                MWWorld::Ptr ptr = mEnvironment.mWorld->getPtrViaHandle (handle);

                if (!ptr.isEmpty())
                    name = MWWorld::Class::get (ptr).getName (ptr);
            }

            if (!name.empty())
                std::cout << "Object: " << name << std::endl;

            focusFrameCounter = 0;
        }

        if (mEnvironment.mWindowManager->getMode()==MWGui::GM_Game)
            mEnvironment.mWorld->doPhysics (movement, mEnvironment.mFrameDuration);
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error in framelistener: " << e.what() << std::endl;
    }

    return true;
}

OMW::Engine::Engine(Cfg::ConfigurationManager& configurationManager)
  : mPhysicEngine (0)
  , mShowFPS (false)
  , mDebug (false)
  , mVerboseScripts (false)
  , mNewGame (false)
  , mUseSound (true)
  , mCompileAll (false)
  , mScriptManager (0)
  , mScriptContext (0)
  , mGuiManager (0)
  , mFSStrict (false)
  , mCfgMgr(configurationManager)
{
    std::srand ( std::time(NULL) );
    MWClass::registerClasses();
}

OMW::Engine::~Engine()
{
    delete mGuiManager;
    delete mEnvironment.mWorld;
    delete mEnvironment.mSoundManager;
    delete mEnvironment.mGlobalScripts;
    delete mEnvironment.mMechanicsManager;
    delete mEnvironment.mDialogueManager;
    delete mEnvironment.mJournal;
    delete mScriptManager;
    delete mScriptContext;
    delete mPhysicEngine;
}

// Load all BSA files in data directory.

void OMW::Engine::loadBSA()
{
    const Files::MultiDirCollection& bsa = mFileCollections.getCollection (".bsa");

    for (Files::MultiDirCollection::TIter iter (bsa.begin()); iter!=bsa.end(); ++iter)
    {
         std::cout << "Adding " << iter->second.string() << std::endl;
         Bsa::addBSA (iter->second.string());
    }

    std::cout << "Data dir " << mDataDir.string() << std::endl;
    Bsa::addDir(mDataDir.string(), mFSStrict);
}

// add resources directory
// \note This function works recursively.

void OMW::Engine::addResourcesDirectory (const boost::filesystem::path& path)
{
    mOgre.getRoot()->addResourceLocation (path.string(), "FileSystem",
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true);
}

void OMW::Engine::enableFSStrict(bool fsStrict)
{
    mFSStrict = fsStrict;
}

// Set data dir

void OMW::Engine::setDataDirs (const Files::PathContainer& dataDirs)
{
    /// \todo remove mDataDir, once resources system can handle multiple directories
    assert (!dataDirs.empty());
    mDataDir = dataDirs.back();
    mFileCollections = Files::Collections (dataDirs, !mFSStrict);
}

// Set resource dir
void OMW::Engine::setResourceDir (const boost::filesystem::path& parResDir)
{
    mResDir = boost::filesystem::system_complete(parResDir);
}

// Set start cell name (only interiors for now)

void OMW::Engine::setCell (const std::string& cellName)
{
    mCellName = cellName;
}

// Set master file (esm)
// - If the given name does not have an extension, ".esm" is added automatically
// - Currently OpenMW only supports one master at the same time.

void OMW::Engine::addMaster (const std::string& master)
{
    assert (mMaster.empty());
    mMaster = master;

    // Append .esm if not already there
    std::string::size_type sep = mMaster.find_last_of (".");
    if (sep == std::string::npos)
    {
        mMaster += ".esm";
    }
}

void OMW::Engine::setDebugMode(bool debugMode)
{
    mDebug = debugMode;
}

void OMW::Engine::setScriptsVerbosity(bool scriptsVerbosity)
{
    mVerboseScripts = scriptsVerbosity;
}

void OMW::Engine::setNewGame(bool newGame)
{
    mNewGame = newGame;
}

// Initialise and enter main loop.

void OMW::Engine::go()
{
    assert (!mEnvironment.mWorld);
    assert (!mCellName.empty());
    assert (!mMaster.empty());

    test.name = "";
    total = 0;

    mOgre.configure(!boost::filesystem::is_regular_file(mCfgMgr.getOgreConfigPath()),
        mCfgMgr.getOgreConfigPath().string(),
        mCfgMgr.getLogPath().string() + std::string("/"),
        mCfgMgr.getPluginsConfigPath().string(), false);

    // This has to be added BEFORE MyGUI is initialized, as it needs
    // to find core.xml here.
    addResourcesDirectory(mResDir / "mygui");

    // Create the window
    mOgre.createWindow("OpenMW");

    loadBSA();

    // Create physics. shapeLoader is deleted by the physic engine
    NifBullet::ManualBulletShapeLoader* shapeLoader = new NifBullet::ManualBulletShapeLoader();
    mPhysicEngine = new OEngine::Physic::PhysicEngine(shapeLoader);

    // Create the world
    mEnvironment.mWorld = new MWWorld::World (mOgre, mPhysicEngine, mFileCollections, mMaster,
        mResDir, mNewGame, mEnvironment, mEncoding);

    // Set up the GUI system
    mGuiManager = new OEngine::GUI::MyGUIManager(mOgre.getWindow(), mOgre.getScene(), false,
        mCfgMgr.getLogPath().string() + std::string("/"));

    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWSkill>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWAttribute>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWSpell>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWSpellEffect>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWDynamicStat>("Widget");

    // Create window manager - this manages all the MW-specific GUI windows
    MWScript::registerExtensions (mExtensions);

    mEnvironment.mWindowManager = new MWGui::WindowManager(mGuiManager->getGui(), mEnvironment,
        mExtensions, mShowFPS, mNewGame);

    // Create sound system
    mEnvironment.mSoundManager = new MWSound::SoundManager(mOgre.getRoot(),
                                                           mOgre.getCamera(),
                                                           mEnvironment.mWorld->getStore(),
                                                           (mDataDir),
                                                           mUseSound, mFSStrict);

    // Create script system
    mScriptContext = new MWScript::CompilerContext (MWScript::CompilerContext::Type_Full,
        mEnvironment);
    mScriptContext->setExtensions (&mExtensions);

    mScriptManager = new MWScript::ScriptManager (mEnvironment.mWorld->getStore(), mVerboseScripts,
        *mScriptContext);

    mEnvironment.mGlobalScripts = new MWScript::GlobalScripts (mEnvironment.mWorld->getStore(),
        *mScriptManager);

    // Create game mechanics system
    mEnvironment.mMechanicsManager = new MWMechanics::MechanicsManager (mEnvironment);

    // Create dialog system
    mEnvironment.mJournal = new MWDialogue::Journal (mEnvironment);
    mEnvironment.mDialogueManager = new MWDialogue::DialogueManager (mEnvironment);

    // load cell
    ESM::Position pos;
    pos.rot[0] = pos.rot[1] = pos.rot[2] = 0;
    pos.pos[2] = 0;

    if (const ESM::Cell *exterior = mEnvironment.mWorld->getExterior (mCellName))
    {
        mEnvironment.mWorld->indexToPosition (exterior->data.gridX, exterior->data.gridY,
            pos.pos[0], pos.pos[1], true);
        mEnvironment.mWorld->changeToExteriorCell (pos);
    }
    else
    {
        pos.pos[0] = pos.pos[1] = 0;
        mEnvironment.mWorld->changeToInteriorCell (mCellName, pos);
    }

    // Sets up the input system
    MWInput::MWInputManager input(mOgre, mEnvironment.mWorld->getPlayer(),
                                  *mEnvironment.mWindowManager, mDebug, *this);
    mEnvironment.mInputManager = &input;

    focusFrameCounter = 0;

    std::cout << "\nPress Q/ESC or close window to exit.\n";

    mOgre.getRoot()->addFrameListener (this);

    // Play some good 'ol tunes
      mEnvironment.mSoundManager->startRandomTitle();

    // scripts
    if (mCompileAll)
    {
        typedef ESMS::ScriptListT<ESM::Script>::MapType Container;

        Container scripts = mEnvironment.mWorld->getStore().scripts.list;

        int count = 0;
        int success = 0;

        for (Container::const_iterator iter (scripts.begin()); iter!=scripts.end(); ++iter, ++count)
            if (mScriptManager->compile (iter->first))
                ++success;

        if (count)
            std::cout
                << "compiled " << success << " of " << count << " scripts ("
                << 100*static_cast<double> (success)/count
                << "%)"
                << std::endl;

    }

    // Start the main rendering loop
    mOgre.start();

    std::cout << "Quitting peacefully.\n";
}

void OMW::Engine::activate()
{
    // TODO: This is only a workaround. The input dispatcher should catch any exceptions thrown inside
    // the input handling functions. Looks like this will require an OpenEngine modification.
    try
    {
        std::string handle = mEnvironment.mWorld->getFacedHandle();

        if (handle.empty())
            return;

        MWWorld::Ptr ptr = mEnvironment.mWorld->getPtrViaHandle (handle);

        if (ptr.isEmpty())
            return;

        MWScript::InterpreterContext interpreterContext (mEnvironment,
            &ptr.getRefData().getLocals(), ptr);

        boost::shared_ptr<MWWorld::Action> action =
            MWWorld::Class::get (ptr).activate (ptr, mEnvironment.mWorld->getPlayer().getPlayer(),
            mEnvironment);

        interpreterContext.activate (ptr, action);

        std::string script = MWWorld::Class::get (ptr).getScript (ptr);

        if (!script.empty())
        {
            mEnvironment.mWorld->getLocalScripts().setIgnore (ptr);
            mScriptManager->run (script, interpreterContext);
        }

        if (!interpreterContext.hasActivationBeenHandled())
        {
            interpreterContext.executeActivation();
        }
    }
    catch (const std::exception& e)
    {
        std::cerr << "Activation failed: " << e.what() << std::endl;
    }
}

void OMW::Engine::setCompileAll (bool all)
{
    mCompileAll = all;
}

void OMW::Engine::setSoundUsage(bool soundUsage)
{
    mUseSound = soundUsage;
}

void OMW::Engine::showFPS(bool showFps)
{
    mShowFPS = showFps;
}

void OMW::Engine::setEncoding(const std::string& encoding)
{
    mEncoding = encoding;
}
