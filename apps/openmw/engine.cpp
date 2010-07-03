#include "engine.hpp"

#include <cassert>

#include <iostream>

#include "components/misc/fileops.hpp"
#include "components/bsa/bsa_archive.hpp"

#include "mwinput/inputmanager.hpp"
#include "mwscript/scriptmanager.hpp"
#include "mwscript/compilercontextscript.hpp"
#include "mwscript/interpretercontext.hpp"
#include "mwscript/extensions.hpp"

#include "mwsound/soundmanager.hpp"

#include "world.hpp"

void OMW::Engine::executeLocalScripts()
{
    for (World::ScriptList::const_iterator iter (mWorld->getLocalScripts().begin());
        iter!=mWorld->getLocalScripts().end(); ++iter)
    {
        MWScript::InterpreterContext interpreterContext (*mWorld, *mSoundManager, iter->second);
        mScriptManager->run (iter->first, interpreterContext);
    }
}

bool OMW::Engine::frameStarted(const Ogre::FrameEvent& evt)
{
    // console
    processCommands();
    
    // local scripts
    executeLocalScripts();

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
: mWorld(NULL), mDebug (false), mSoundManager (0), mScriptManager (0), mScriptContext (0)
{
    mspCommandServer.reset(
        new OMW::CommandServer::Server(&mCommandQueue, kCommandServerPort));
}

OMW::Engine::~Engine()
{
//    mspCommandServer->stop();
    delete mWorld;
    delete mSoundManager;
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

// Initialise and enter main loop.

void OMW::Engine::go()
{
    assert (!mWorld);
    assert (!mDataDir.empty());
    assert (!mCellName.empty());
    assert (!mMaster.empty());

    std::cout << "Hello, fellow traveler!\n";

    std::cout << "Your data directory for today is: " << mDataDir << "\n";

    std::cout << "Initializing OGRE\n";

    const char* plugCfg = "plugins.cfg";

    mOgre.configure(!isFile("ogre.cfg"), plugCfg, false);

    addResourcesDirectory (mDataDir / "Meshes");
    addResourcesDirectory (mDataDir / "Textures");

    // Create the window
    mOgre.createWindow("OpenMW");

    loadBSA();

    // Create the world
    mWorld = new World (mOgre, mDataDir, mMaster, mCellName);

    mSoundManager = new MWSound::SoundManager;

    MWScript::registerExtensions (mExtensions);

    mScriptContext = new MWScript::CompilerContextScript;
    mScriptContext->setExtensions (&mExtensions);

    mScriptManager = new MWScript::ScriptManager (mWorld->getStore(), mVerboseScripts,
        *mScriptContext);

    std::cout << "Setting up input system\n";

    // Sets up the input system
    MWInput::MWInputManager input(mOgre, mWorld->getPlayerPos(), mDebug);

    // Launch the console server
    std::cout << "Starting command server on port " << kCommandServerPort << std::endl;
    mspCommandServer->start();

    std::cout << "\nStart! Press Q/ESC or close window to exit.\n";

    mOgre.getRoot()->addFrameListener (this);

    // Start the main rendering loop
    mOgre.start();

    std::cout << "\nThat's all for now!\n";
}


