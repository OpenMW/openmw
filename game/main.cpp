#include <iostream>

#include "cell_store.hpp"
#include "render/cell.hpp"
#include "render/mwscene.hpp"
#include "bsa/bsa_archive.hpp"
#include "ogre/renderer.hpp"
#include "tools/fileops.hpp"
#include "input/oismanager.hpp"
#include "input/listener.hpp"

using namespace std;

void maintest()
{
  const char* esmFile = "data/Morrowind.esm";
  const char* bsaFile = "data/Morrowind.bsa";

#ifdef _WIN32
  const char* plugCfg = "plugins.cfg.win32";
#else
  const char* plugCfg = "plugins.cfg.linux";
#endif

  cout << "Hello, fellow traveler!\n";

  cout << "Initializing OGRE\n";
  Render::OgreRenderer ogre;
  ogre.configure(!isFile("ogre.cfg"), plugCfg, false);

  cout << "Adding " << bsaFile << endl;
  addBSA(bsaFile);

  cout << "Loading ESM " << esmFile << "\n";
  ESM::ESMReader esm;
  ESMS::ESMStore store;
  ESMS::CellStore cell;

  // This parses the ESM file and loads a sample cell
  esm.open(esmFile);
  store.load(esm);
  cell.loadInt("Beshara", store, esm);

  // Create the window
  ogre.createWindow("OpenMW");

  cout << "\nSetting up cell rendering (not done)\n";

  // Sets up camera, scene manager etc
  Render::MWScene scene(ogre);

  // This doesn't do anything yet.
  Render::CellRender rend(cell);

  cout << "Setting up input system\n";

  // Sets up the input system
  Input::OISManager input(ogre);

  // Add the frame and input listener
  Input::ExitListener frame(ogre, input);

  cout << "\nStart! Press Q/ESC or close window to exit.\n";

  // Start the main rendering loop
  ogre.start();

  cout << "\nThat's all for now!\n";
}

int main(/*int argc, char**argv*/)
{
  try { maintest(); }
  catch(exception &e)
    {
      cout << "\nERROR: " << e.what() << endl;
      return 1;
    }
  return 0;
}
