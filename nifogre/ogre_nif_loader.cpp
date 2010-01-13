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
#include "../nif/node.h"

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

static void handleNiTriShape(Mesh *mesh, NiTriShape *shape, int flags)
{
  // Interpret flags
  bool hidden    = (flags & 0x01) != 0; // Not displayed
  bool collide   = (flags & 0x02) != 0; // Use mesh for collision
  bool bbcollide = (flags & 0x04) != 0; // Use bounding box for collision

  // Bounding box collision isn't implemented, always use mesh for now.
  if(bbcollide)
    {
      collide = true;
      bbcollide = false;
    }

  // If the object was marked "NCO" earlier, it shouldn't collide with
  // anything.
  if(flags & 0x800)
    { collide = false; bbcollide = false; }

  // Skip the entire material phase for hidden nodes
  if(hidden) goto nomaterial;

  nomaterial:
}

static void handleNode(Mesh* mesh, Node *node, int flags)
{
  // Accumulate the flags from all the child nodes. This works for all
  // the flags we currently use, at least.
  flags |= node->flags;

  // Check for extra data
  Extra *e = node;
  while(!e->extra.empty())
    {
      // Get the next extra data in the list
      e = e.extra.getPtr();
      assert(e != NULL);

      if(e->recType == RC_NiStringExtraData)
        {
          // String markers may contain important information
          // affecting the entire subtree of this node
          NiStringExtraData *sd = (NiStringExtraData*)e;

          if(sd->string == "NCO")
            // No collision. Use an internal flag setting to mark this.
            flags |= 0x800;
          else if(sd->string == "MRK")
            // Marker objects. These are only visible in the
            // editor. Until and unless we add an editor component to
            // the engine, just skip this entire node.
            return;
        }
    }

  // For NiNodes, loop through children
  if(node->recType == RC_NiNode)
    {
      NodeList &list = ((NiNode*)node)->children;
      int n = list.length();
      for(int i=0; i<n; i++)
        {
          if(list.has(i))
            handleNode(mesh, &list[i], flags);
        }
    }
  else if(node->recType == RC_NiTriShape)
    // For shapes
    handleNiTriShape(mesh, (NiTriShape*)node, flags);
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
  NIFFile nif(vfs->open(name), name);

  if(nif.numRecords() < 1)
    {
      warn("Found no records in " + name);
      return;
    }

  // The first record is assumed to be the root node
  Record *r = nif.getRecord(0);
  assert(r != NULL);

  if(r->recType != RC_NiNode)
    {
      warn("First record in " + name + " was not a NiNode, but a " +
           r->recName.toString() + ". Skipping file.");
      return;
    }

  // Handle the node
  handleNode(mesh, (Node*)r, 0);
}

MeshPtr NIFLoader::load(const char* name, const char* group)
{
  MeshManager *m = MeshManager::getSingletonPtr();

  // Check if the resource already exists
  ResourcePtr ptr = m->getByName(name/*, group*/);
  if(!ptr.isNull())
    return MeshPtr(ptr);

  // Nope, create a new one.
  return MeshManager::getSingleton().createManual(name, group, &g_sing);
}
