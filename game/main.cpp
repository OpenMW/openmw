#include <iostream>

#include "esm_store/cell_store.hpp"
#include "bsa/bsa_archive.hpp"
#include "ogre/renderer.hpp"
#include "tools/fileops.hpp"

#include "mwrender/cell.hpp"
#include "mwrender/mwscene.hpp"
#include "mwinput/inputmanager.hpp"

using namespace std;

void maintest()
{
  const char* esmFile = "data/Morrowind.esm";
  const char* bsaFile = "data/Morrowind.bsa";

  const char* plugCfg = "plugins.cfg";

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

  cout << "\nSetting up cell rendering\n";

  // Sets up camera, scene manager etc
  MWRender::MWScene scene(ogre);

  // This connects the cell data with the rendering scene.
  MWRender::CellRender rend(cell, scene);

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
