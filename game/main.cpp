#include <iostream>

#include "cell_store.hpp"
#include "render/cell.hpp"
#include "render/mwscene.hpp"
#include "bsa/bsa_archive.hpp"
#include "ogre/renderer.hpp"
#include "tools/fileops.hpp"
#include "input/oismanager.hpp"

using namespace std;

void maintest()
{
  const char* esmFile = "../data/Morrowind.esm";
  const char* bsaFile = "../data/Morrowind.bsa";

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
  Render::CellRender rend(cell);

  esm.open(esmFile);
  store.load(esm);
  cell.loadInt("Beshara", store, esm);

  ogre.createWindow("OpenMW");

  Render::MWScene scene;
  scene.setup(&ogre);

  Input::OISManager input;
  input.setup(&ogre);

  // Add the frame listener
  //root->addFrameListener(&mFrameListener);

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
