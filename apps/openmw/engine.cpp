#include "engine.hpp"

#include <cassert>

#include <iostream>

#include "components/esm_store/cell_store.hpp"
#include "components/bsa/bsa_archive.hpp"
#include "components/engine/ogre/renderer.hpp"
#include "libs/platform/fileops.hpp"

#include "apps/openmw/mwrender/interior.hpp"
#include "mwinput/inputmanager.hpp"
#include "apps/openmw/mwrender/playerpos.hpp"
#include "apps/openmw/mwrender/sky.hpp"

OMW::Engine::Engine() 
    : mEnableSky   (false)
    , mpSkyManager (NULL)
{
}

// adjust name and load bsa

void OMW::Engine::prepareMaster()
{
    std::string::size_type sep = mMaster.find_last_of (".");

    if (sep==std::string::npos)
    {
        mMaster += ".esm";
    }
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
}

// Enables sky rendering
//
void OMW::Engine::enableSky (bool bEnable)
{
    mEnableSky = bEnable;
}

// Initialise and enter main loop.

void OMW::Engine::go()
{
    assert (!mDataDir.empty());
    assert (!mCellName.empty());
    assert (!mMaster.empty());

    std::cout << "Hello, fellow traveler!\n";

    std::cout << "Your data directory for today is: " << mDataDir << "\n";

    std::cout << "Initializing OGRE\n";

    const char* plugCfg = "plugins.cfg";

    mOgre.configure(!OMW::Platform::isFile("ogre.cfg"), plugCfg, false);

    addResourcesDirectory (mDataDir / "Meshes");
    addResourcesDirectory (mDataDir / "Textures");

    prepareMaster();
    loadBSA();

    boost::filesystem::path masterPath (mDataDir);
    masterPath /= mMaster;

    std::cout << "Loading ESM " << masterPath.string() << "\n";
    ESM::ESMReader esm;
    ESMS::ESMStore store;
    ESMS::CellStore cell;

    // This parses the ESM file and loads a sample cell
    esm.open(masterPath.file_string());
    store.load(esm);

    cell.loadInt(mCellName, store, esm);

    // Create the window
    mOgre.createWindow("OpenMW");

    std::cout << "\nSetting up cell rendering\n";

    // Sets up camera, scene manager, and viewport.
    MWRender::MWScene scene(mOgre);

    // Used to control the player camera and position
    MWRender::PlayerPos player(scene.getCamera());

    // This connects the cell data with the rendering scene.
    MWRender::InteriorCellRender rend(cell, scene);

    // Load the cell and insert it into the renderer
    rend.show();

    // Optionally enable the sky
    if (mEnableSky)
        mpSkyManager = MWRender::SkyManager::create(mOgre.getWindow(), scene.getCamera());

    std::cout << "Setting up input system\n";

    // Sets up the input system
    MWInput::MWInputManager input(mOgre, player);

    std::cout << "\nStart! Press Q/ESC or close window to exit.\n";

    // Start the main rendering loop
    mOgre.start();

    delete mpSkyManager;

    std::cout << "\nThat's all for now!\n";
}

