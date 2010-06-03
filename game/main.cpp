#include <iostream>

#include "cell_store.hpp"
#include "render/cell.hpp"
#include "bsa/bsa_archive.hpp"
#include <Ogre.h>

using namespace std;

// Absolute minimal OGRE setup
void ogre_setup()
{
  using namespace Ogre;

  // Disable Ogre logging
  new LogManager;
  Log *log = LogManager::getSingleton().createLog("");
  log->setDebugOutputEnabled(false);

  // Set up Root.
  new Root();
}

void main_setup(const char* bsaFile)
{
  cout << "Hello, fellow traveler!\n";

  cout << "Initializing OGRE\n";
  ogre_setup();

  cout << "Adding " << bsaFile << endl;
  addBSA(bsaFile);
}

void maintest()
{
  const char* esmFile = "data/Morrowind.esm";
  const char* bsaFile = "data/Morrowind.bsa";

  main_setup(bsaFile);

  cout << "Loading ESM " << esmFile << "\n";
  ESM::ESMReader esm;
  ESMS::ESMStore store;
  ESMS::CellStore cell;
  Render::CellRender rend(cell);

  esm.open(esmFile);
  store.load(esm);
  cell.loadInt("Beshara", store, esm);

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
