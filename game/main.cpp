#include <iostream>

#include "cell_store.hpp"
#include "render/cell.hpp"
#include "bsa/bsa_archive.hpp"
#include "ogre/renderer.hpp"
#include "tools/fileops.hpp"

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
  Render::CellRender rend(cell);

  esm.open(esmFile);
  store.load(esm);
  cell.loadInt("Beshara", store, esm);

  ogre.createWindow("OpenMW");

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
