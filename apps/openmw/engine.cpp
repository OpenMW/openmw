#include "engine.hpp"
#include "components/esm/loadcell.hpp"

#include <cassert>

#include <iostream>
#include <utility>

#include "components/esm/records.hpp"
#include <components/esm_store/cell_store.hpp>
#include <components/misc/fileops.hpp>
#include <components/bsa/bsa_archive.hpp>
#include <components/esm/loadregn.hpp>
#include <components/esm/esm_reader.hpp>
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

#include "mwclass/classes.hpp"

#include "mwdialogue/dialoguemanager.hpp"

#include "mwmechanics/mechanicsmanager.hpp"

#include <OgreRoot.h>

#include <MyGUI_WidgetManager.h>
#include "mwgui/class.hpp"


//using namespace ESM;

void OMW::Engine::executeLocalScripts()
{
    for (MWWorld::World::ScriptList::const_iterator iter (
        mEnvironment.mWorld->getLocalScripts().begin());
        iter!=mEnvironment.mWorld->getLocalScripts().end(); ++iter)
    {
        if (mIgnoreLocalPtr.isEmpty() || mIgnoreLocalPtr!=iter->second)
        {
            MWScript::InterpreterContext interpreterContext (mEnvironment,
                &iter->second.getRefData().getLocals(), MWWorld::Ptr (iter->second));
            mScriptManager->run (iter->first, interpreterContext);

            if (mEnvironment.mWorld->hasCellChanged())
                break;
        }
    }

    mIgnoreLocalPtr = MWWorld::Ptr();
}


bool OMW::Engine::frameStarted(const Ogre::FrameEvent& evt)
{
	if(! (mEnvironment.mSoundManager->isMusicPlaying()))
	{
		// Play some good 'ol tunes
			mEnvironment.mSoundManager->startRandomTitle();
	}

	std::string effect;



	MWWorld::Ptr::CellStore *current = mEnvironment.mWorld->getPlayerPos().getPlayer().getCell();
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

			    srand ( time(NULL) );
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

    try
    {
        mEnvironment.mFrameDuration = evt.timeSinceLastFrame;

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
        mEnvironment.mMechanicsManager->update();

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
    }
    catch (const std::exception& e)
    {
        std::cerr << "Error in framelistener: " << e.what() << std::endl;
    }
	//std::cout << "TESTING2";

    return true;
}

OMW::Engine::Engine()
  : mDebug (false)
  , mVerboseScripts (false)
  , mNewGame (false)
  , mUseSound (true)
  , mCompileAll (false)
  , mScriptManager (0)
  , mScriptContext (0)
  , mGuiManager (0)
{
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
    delete mScriptManager;
    delete mScriptContext;
}

// Load all BSA files in data directory.

void OMW::Engine::loadBSA()
{
    boost::filesystem::directory_iterator end;

    for (boost::filesystem::directory_iterator iter (mDataDir); iter!=end; ++iter)
    {
        if (boost::filesystem::extension (iter->path())==".bsa")
        {
            std::cout << "Adding " << iter->path().string() << std::endl;
            addBSA(iter->path().file_string());
        }
    }
}

// add resources directory
// \note This function works recursively.

void OMW::Engine::addResourcesDirectory (const boost::filesystem::path& path)
{
    mOgre.getRoot()->addResourceLocation (path.file_string(), "FileSystem",
        Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME, true);
}

// Set data dir

void OMW::Engine::setDataDir (const boost::filesystem::path& dataDir)
{
    mDataDir = boost::filesystem::system_complete (dataDir);
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

void OMW::Engine::enableDebugMode()
{
    mDebug = true;
}

void OMW::Engine::enableVerboseScripts()
{
    mVerboseScripts = true;
}

void OMW::Engine::setNewGame()
{
    mNewGame = true;
}

// Initialise and enter main loop.

void OMW::Engine::go()
{
    assert (!mEnvironment.mWorld);
    assert (!mDataDir.empty());
    assert (!mCellName.empty());
    assert (!mMaster.empty());

	test.name = "";
	total = 0;



    std::cout << "Data directory: " << mDataDir << "\n";

    const char* plugCfg = "plugins.cfg";

    mOgre.configure(!isFile("ogre.cfg"), plugCfg, false);

    addResourcesDirectory (mDataDir / "Meshes");
    addResourcesDirectory (mDataDir / "Textures");

    // This has to be added BEFORE MyGUI is initialized, as it needs
    // to find core.xml here.
    addResourcesDirectory("resources/mygui/");

    // Create the window
    mOgre.createWindow("OpenMW");

    loadBSA();

    // Create the world
    mEnvironment.mWorld = new MWWorld::World (mOgre, mDataDir, mMaster, mNewGame, mEnvironment);

    // Set up the GUI system
    mGuiManager = new OEngine::GUI::MyGUIManager(mOgre.getWindow(),
                                                 mOgre.getScene());
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWSkill>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWAttribute>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWSpell>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWSpellEffect>("Widget");
    MyGUI::FactoryManager::getInstance().registerFactory<MWGui::Widgets::MWDynamicStat>("Widget");

    // Create window manager - this manages all the MW-specific GUI windows
    MWScript::registerExtensions (mExtensions);

    mEnvironment.mWindowManager = new MWGui::WindowManager(mGuiManager->getGui(), mEnvironment,
        mExtensions, mNewGame);

    // Create sound system
    mEnvironment.mSoundManager = new MWSound::SoundManager(mOgre.getRoot(),
                                                           mOgre.getCamera(),
                                                           mEnvironment.mWorld->getStore(),
                                                           (mDataDir),
                                                           mUseSound);

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
        mEnvironment.mWorld->changeCell (mCellName, pos);
    }

    // Sets up the input system
    MWInput::MWInputManager input(mOgre, mEnvironment.mWorld->getPlayerPos(),
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
            MWWorld::Class::get (ptr).activate (ptr, mEnvironment.mWorld->getPlayerPos().getPlayer(),
            mEnvironment);

        interpreterContext.activate (ptr, action);

        std::string script = MWWorld::Class::get (ptr).getScript (ptr);

        if (!script.empty())
        {
            mIgnoreLocalPtr = ptr;
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
