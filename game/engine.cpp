#include "engine.hpp"

#include <cassert>

#include <iostream>

#include "esm_store/cell_store.hpp"
#include "bsa/bsa_archive.hpp"
#include "ogre/renderer.hpp"
#include "tools/fileops.hpp"

#include "mwrender/interior.hpp"
#include "mwinput/inputmanager.hpp"
#include "mwrender/playerpos.hpp"

OMW::Engine::Engine() {}

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
#if OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	mDataDir = boost::filesystem::system_complete (macBundlePath() + "/Contents/MacOS/" + dataDir.directory_string());
#else
    mDataDir = boost::filesystem::system_complete (dataDir);	
#endif

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

// Initialise and enter main loop.

void OMW::Engine::go()
{
    assert (!mDataDir.empty());
    assert (!mCellName.empty());
    assert (!mMaster.empty());
    
    std::cout << "Hello, fellow traveler!\n";
	
    std::cout << "Your data directory for today is: " << mDataDir << "\n";

    std::cout << "Initializing OGRE\n";
	
	std::string plugin_cfg_location = "plugins.cfg";
	std::string ogre_cfg_location = "ogre.cfg";

#ifdef OGRE_PLATFORM == OGRE_PLATFORM_APPLE
	std::cout << "[Apple]" << std::endl;
	plugin_cfg_location = macBundlePath() + "/Contents/MacOS/" + plugin_cfg_location;
	ogre_cfg_location =   macBundlePath() + "/Contents/MacOS/" + ogre_cfg_location;
#endif

    mOgre.configure(!isFile(ogre_cfg_location.c_str()), plugin_cfg_location.c_str(), false);
    
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

    std::cout << "Setting up input system\n";

    // Sets up the input system
    MWInput::MWInputManager input(mOgre, player);

    std::cout << "\nStart! Press Q/ESC or close window to exit.\n";

    // Start the main rendering loop
    mOgre.start();

    std::cout << "\nThat's all for now!\n";        
}

