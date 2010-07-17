#include "engine.hpp"

#include <cassert>

#include <iostream>

#include <components/misc/fileops.hpp>
#include <components/bsa/bsa_archive.hpp>

#include <openengine/gui/manager.hpp>
#include <components/mwgui/window_manager.hpp>

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

#include <OgreRoot.h>

void OMW::Engine::executeLocalScripts()
{
    for (MWWorld::World::ScriptList::const_iterator iter (
        mEnvironment.mWorld->getLocalScripts().begin());
        iter!=mEnvironment.mWorld->getLocalScripts().end(); ++iter)
    {
        MWScript::InterpreterContext interpreterContext (mEnvironment,
            &iter->second.getRefData().getLocals(), MWWorld::Ptr (iter->second));
        mScriptManager->run (iter->first, interpreterContext);
    }
}

bool OMW::Engine::frameStarted(const Ogre::FrameEvent& evt)
{
    mEnvironment.mFrameDuration = evt.timeSinceLastFrame;

    // console
    processCommands();
    
    // local scripts
    executeLocalScripts();
    
    // global scripts
    mEnvironment.mGlobalScripts->run (mEnvironment);

    return true;
}
            
void OMW::Engine::processCommands()
{
    Command cmd;
    while (mCommandQueue.try_pop_front(cmd))
    {
        ///\todo Add actual processing of the received command strings
        std::cout << "Command: '" << cmd.mCommand << "'" << std::endl;

        ///\todo Replace with real output.  For now, echo back the string in uppercase
        std::string reply(cmd.mCommand);
        std::transform(reply.begin(), reply.end(), reply.begin(), toupper);
        cmd.mReplyFunction(reply);
    } 
}

OMW::Engine::Engine()
  : mDebug (false)
  , mVerboseScripts (false)
  , mNewGame (false)
  , mEnableCommandServer (false)
  , mScriptManager (0)
  , mScriptContext (0)
{   
}

OMW::Engine::~Engine()
{
    if (mspCommandServer.get())
        mspCommandServer->stop();

    delete mGuiManager;
    delete mEnvironment.mWorld;
    delete mEnvironment.mSoundManager;
    delete mEnvironment.mGlobalScripts;
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

void OMW::Engine::enableCommandServer()
{
    mEnableCommandServer = true;
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
    mEnvironment.mWorld = new MWWorld::World (mOgre, mDataDir, mMaster, mCellName, mNewGame);

    // Set up the GUI system
    mGuiManager = new OEngine::GUI::MyGUIManager(mOgre.getWindow(),
                                                 mOgre.getScene());

    // Create window manager - this manages all the MW-specific GUI windows
    mEnvironment.mWindowManager = new MWGui::WindowManager(mGuiManager->getGui());

    mEnvironment.mSoundManager = new MWSound::SoundManager;

    MWScript::registerExtensions (mExtensions);

    mScriptContext = new MWScript::CompilerContext (MWScript::CompilerContext::Type_Full,
        mEnvironment);
    mScriptContext->setExtensions (&mExtensions);

    mScriptManager = new MWScript::ScriptManager (mEnvironment.mWorld->getStore(), mVerboseScripts,
        *mScriptContext);
        
    mEnvironment.mGlobalScripts = new MWScript::GlobalScripts (mEnvironment.mWorld->getStore(),
        *mScriptManager);

    // Sets up the input system
    MWInput::MWInputManager input(mOgre, mEnvironment.mWorld->getPlayerPos(),
                                  *mEnvironment.mWindowManager, mDebug);

    // Launch the console server
    if (mEnableCommandServer)
    {
        std::cout << "Starting command server on port " << kCommandServerPort << std::endl;
        mspCommandServer.reset(new OMW::CommandServer::Server(&mCommandQueue, kCommandServerPort));
        mspCommandServer->start();
    }
    else
        std::cout << "Command server disabled" << std::endl;

    std::cout << "\nPress Q/ESC or close window to exit.\n";

    mOgre.getRoot()->addFrameListener (this);

    // Start the main rendering loop
    mOgre.start();

    std::cout << "Quitting peacefully.\n";
}


