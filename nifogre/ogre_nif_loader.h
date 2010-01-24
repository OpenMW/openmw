/*
  OpenMW - The completely unofficial reimplementation of Morrowind
  Copyright (C) 2008-2010  Nicolay Korslund
  Email: < korslund@gmail.com >
  WWW: http://openmw.sourceforge.net/

  This file (ogre_nif_loader.h) is part of the OpenMW package.

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

#ifndef _OGRE_NIF_LOADER_H_
#define _OGRE_NIF_LOADER_H_

#include <OgreResource.h>
#include <OgreMesh.h>
#include <assert.h>

/** Manual resource loader for NIF meshes. This is the main class
    responsible for translating the internal NIF mesh structure into
    something Ogre can use. Later it will also handle the insertion of
    collision meshes into Bullet / OgreBullet.

    You have to insert meshes manually into Ogre like this:

    NIFLoader::load("somemesh.nif");

    Afterwards, you can use the mesh name "somemesh.nif" normally to
    create entities and so on. The mesh isn't loaded from disk until
    OGRE needs it for rendering. Thus the above load() command is not
    very resource intensive.
 */
struct NIFLoader : Ogre::ManualResourceLoader
{
  void loadResource(Ogre::Resource *resource);

  static Ogre::MeshPtr load(const char* name, const char* group="General");
};

#endif
