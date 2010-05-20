/* Split off into a separate file just to increase compile
   speed. Parsing Ogre.h takes a long time, and the Ogre-dependent
   part doesn't change much. This entire layout will change later.
 */

#include <iostream>
#include "bsa/bsa_archive.h"
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

void main_setup(const char* bsaFile)
{
  cout << "Hello, fellow traveler!\n";

  cout << "Initializing OGRE\n";
  ogre_setup();

  cout << "Adding " << bsaFile << endl;
  addBSA(bsaFile);
}

