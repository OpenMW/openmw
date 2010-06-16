#include "engine.hpp"

#include <cassert>

#include <iostream>

#include "esm_store/cell_store.hpp"
#include "bsa/bsa_archive.hpp"
#include "ogre/renderer.hpp"
#include "tools/fileops.hpp"

#include "mwrender/interior.hpp"
#include "mwinput/inputmanager.hpp"

OMW::Engine::Engine() {}

// adjust name and load bsa

void OMW::Engine::prepareMaster()
{
    assert (!mDataDir.empty());
    
    std::string masterName; // name without extension

    std::string::size_type sep = mMaster.find_last_of (".");
    
    if (sep==std::string::npos)
    {
        masterName = mMaster;
        mMaster += ".esm";
    }
    else
    {
        masterName = mMaster.substr (0, sep);
    }

    // bsa
    boost::filesystem::path bsa (mDataDir);
    bsa /= masterName + ".bsa";

    if (boost::filesystem::exists (bsa))
    {
        std::cout << "Adding " << bsa.string() << std::endl;
        addBSA(bsa.file_string());
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
// - If there is a bsa file with the same name, OpenMW will load it.
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

    const char* plugCfg = "plugins.cfg";

    mOgre.configure(!isFile("ogre.cfg"), plugCfg, false);
    
    addResourcesDirectory (mDataDir / "Meshes");
    addResourcesDirectory (mDataDir / "Textures");
    
    prepareMaster();
    
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

    // Sets up camera, scene manager etc
    MWRender::MWScene scene(mOgre);

    // This connects the cell data with the rendering scene.
    MWRender::InteriorCellRender rend(cell, scene);

    // Load the cell and insert it into the renderer
    rend.show();

    std::cout << "Setting up input system\n";

    // Sets up the input system
    MWInput::MWInputManager input(mOgre);

    std::cout << "\nStart! Press Q/ESC or close window to exit.\n";

    // Start the main rendering loop
    mOgre.start();

    std::cout << "\nThat's all for now!\n";        
}

