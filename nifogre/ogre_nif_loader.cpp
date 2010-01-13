/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (ogre_nif_loader.cpp) is part of the OpenMW package.

  OpenMW is distributed as free software: you can redistribute it
  and/or modify it under the terms of the GNU General Public License
  version 3, as published by the Free Software Foundation.

  This program is distributed in the hope that it will be useful, but
  WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
  General Public License for more details.

  You should have received a copy of the GNU General Public License
  version 3 along with this program. If not, see
  http://www.gnu.org/licenses/ .

 */

#include "ogre_nif_loader.h"
#include <Ogre.h>

#include "../mangle/vfs/servers/ogre_vfs.h"
#include "../nif/nif_file.h"

// For warning messages
#include <iostream>

using namespace std;
using namespace Ogre;
using namespace Nif;
using namespace Mangle::VFS;

// This is the interface to the Ogre resource system. It allows us to
// load NIFs from BSAs, in the file system and in any other place we
// tell Ogre to look (eg. in zip or rar files.)
OgreVFS *vfs;

// Singleton instance used by load()
static NIFLoader g_sing;

static void warn(const string &msg)
{
  cout << "WARNING (NIF): " << msg << endl;
}

void NIFLoader::loadResource(Resource *resource)
{
  // Set up the VFS if it hasn't been done already
  if(!vfs) vfs = new OgreVFS("General");

  // Get the mesh
  Mesh *mesh = dynamic_cast<Mesh*>(resource);
  assert(mesh);

  // Look it up
  const String &name = mesh->getName();
  if(!vfs->isFile(name))
    {
      warn("File not found: " + name);
      return;
    }

  // Load the NIF
  cout << "Loading " << name << endl;
  NIFFile nif(vfs->open(name), name);

  int n = nif.numRecords();
  cout << "Number of records: " << n << endl;
  if(n)
    cout << "First record type: " << nif.getRecord(0)->recName.toString() << endl;
}

MeshPtr NIFLoader::load(const char* name, const char* group)
{
  return MeshManager::getSingleton().createManual(name, group, &g_sing);
}
