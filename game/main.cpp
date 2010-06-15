#include <iostream>

#include <string>
#include <fstream>

#include <boost/program_options.hpp>
#include <boost/filesystem.hpp>

#include "esm_store/cell_store.hpp"
#include "bsa/bsa_archive.hpp"
#include "ogre/renderer.hpp"
#include "tools/fileops.hpp"

#include "mwrender/interior.hpp"
#include "mwrender/mwscene.hpp"
#include "mwinput/inputmanager.hpp"

using namespace std;

void maintest (boost::filesystem::path dataDir, const std::string& cellName,
    std::string master)
{
    assert (!dataDir.empty());
  
    dataDir = boost::filesystem::system_complete (dataDir);

    std::string masterName; // name without extension
    
    std::string::size_type sep = master.find_last_of (".");
    
    if (sep==std::string::npos)
    {
        masterName = master;
        master += ".esm";
    }
    else
    {
        masterName = master.substr (0, sep);
    }

    boost::filesystem::path masterPath (dataDir);
    masterPath /= master;

    boost::filesystem::path bsa (dataDir);
    bsa /= masterName + ".bsa";

    const char* plugCfg = "plugins.cfg";

    cout << "Hello, fellow traveler!\n";
  
    cout << "Your data directory for today is: " << dataDir << "\n";

    cout << "Initializing OGRE\n";
    Render::OgreRenderer ogre;
    ogre.configure(!isFile("ogre.cfg"), plugCfg, false);

    if (boost::filesystem::exists (bsa))
    {
        cout << "Adding " << bsa.string() << endl;
        addBSA(bsa.file_string());
    }

    cout << "Loading ESM " << masterPath.string() << "\n";
    ESM::ESMReader esm;
    ESMS::ESMStore store;
    ESMS::CellStore cell;

    // This parses the ESM file and loads a sample cell
    esm.open(masterPath.file_string());
    store.load(esm);

    cell.loadInt(cellName, store, esm);

    // Create the window
    ogre.createWindow("OpenMW");

    cout << "\nSetting up cell rendering\n";

    // Sets up camera, scene manager etc
    MWRender::MWScene scene(ogre);

    // This connects the cell data with the rendering scene.
    MWRender::InteriorCellRender rend(cell, scene);

    // Load the cell and insert it into the renderer
    rend.show();

    cout << "Setting up input system\n";

    // Sets up the input system
    MWInput::MWInputManager input(ogre);

    cout << "\nStart! Press Q/ESC or close window to exit.\n";

    // Start the main rendering loop
    ogre.start();

    cout << "\nThat's all for now!\n";
}

int main(int argc, char**argv)
{
  try
  {
    boost::program_options::options_description desc (
      "Syntax: openmw <options>\nAllowed options");

    desc.add_options()
      ("help", "print help message")
      ("data", boost::program_options::value<std::string>()->default_value ("data"),
        "set data directory")
      ("start", boost::program_options::value<std::string>()->default_value ("Beshara"),
        "set initial cell (only interior cells supported at the moment")
      ("master", boost::program_options::value<std::string>()->default_value ("Morrowind"),
        "master file")
      ;
  
    boost::program_options::variables_map variables;
    
    std::ifstream configFile ("openmw.cfg");

    boost::program_options::store (
      boost::program_options::parse_command_line (argc, argv, desc), variables);
    boost::program_options::notify (variables);

    if (configFile.is_open())
      boost::program_options::store (
        boost::program_options::parse_config_file (configFile, desc), variables);

    if (variables.count ("help"))
    {
      std::cout << desc << std::endl;
    }
    else
    {          
      maintest (variables["data"].as<std::string>(), variables["start"].as<std::string>(),
        variables["master"].as<std::string>());
    }  
  }
  catch(exception &e)
    {
      cout << "\nERROR: " << e.what() << endl;
      return 1;
    }
  return 0;
}
