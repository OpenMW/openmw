#include <iostream>

#include "bsa/bsa_archive.h"
#include "esm/records.hpp"

#include "Ogre.h"

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

void maintest()
{
  const char* bsaFile = "data/Morrowind.bsa";
  const char* esmFile = "data/Morrowind.esm";

  cout << "Hello, fellow traveler!\n";

  cout << "Initializing OGRE\n";
  ogre_setup();

  cout << "Adding " << bsaFile << endl;
  addBSA(bsaFile);

  cout << "Loading ESM " << esmFile << " (header only)\n";
  ESM::ESMReader esm;
  esm.open(esmFile);

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
