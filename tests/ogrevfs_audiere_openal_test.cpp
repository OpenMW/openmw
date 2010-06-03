/*
  This example combines:

  - the OGRE VFS system (to read from zip)
  - Audiere (for decoding sound data)
  - OpenAL (for sound playback)

 */

#include "sound/filters/openal_audiere.hpp"
#include "vfs/servers/ogre_vfs.hpp"
#include <Ogre.h>
#include <iostream>

using namespace Ogre;
using namespace Mangle;
using namespace std;

int main()
{
  // Disable Ogre logging
  new LogManager;
  Log *log = LogManager::getSingleton().createLog("");
  log->setDebugOutputEnabled(false);

  // Set up Root
  Root *root = new Root("","","");

  // Add zip file with a sound in it
  root->addResourceLocation("sound.zip", "Zip", "General");

  // Ogre file system
  VFS::OgreVFS vfs;

  // The main sound system
  Sound::OpenAL_Audiere_Factory mg;
  Sound::SoundPtr snd = mg.load(vfs.open("owl.ogg"));

  cout << "Playing 'owl.ogg' from 'sound.zip'\n";
  snd->play();

  while(snd->isPlaying())
    {
      usleep(10000);
      if(mg.needsUpdate) mg.update();
    }

  return 0;
}
