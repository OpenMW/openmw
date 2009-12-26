/*
  This example combines:

  - the OGRE VFS system (to read from zip)
  - Audiere (for decoding sound data)
  - OpenAL (for sound playback)

 */

#include "sound/imp/openal_audiere.h"
#include "vfs/imp_server/ogre_vfs.h"
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

  Sound::OpenAL_Audiere_Manager mg;
  Sound::Sound *snd = mg.load(vfs.open("owl.ogg"));

  Sound::Instance *s = snd->getInstance(false, false);
  cout << "Playing 'owl.ogg' from 'sound.zip'\n";
  s->play();

  while(s->isPlaying())
    {
      usleep(10000);
      if(mg.needsUpdate) mg.update();
    }

  if(s) s->drop();
  if(snd) snd->drop();

  return 0;
}
