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
#include <OgreSkeleton.h>

#include <vector>
#include <string>

namespace Nif
{
    class Node;
    class Transformation;
    class NiTriShape;
}

namespace NifOgre
{

// FIXME: These should not be in NifOgre, it works agnostic of what model format is used
typedef std::multimap<float,std::string> TextKeyMap;
struct EntityList {
    std::vector<Ogre::Entity*> mEntities;
    Ogre::Entity *mSkelBase;

    EntityList() : mSkelBase(0)
    { }
};


/** This holds a list of mesh names along with the names of their parent nodes */
typedef std::vector< std::pair<std::string,std::string> > MeshPairList;

/** Manual resource loader for NIF meshes. This is the main class
    responsible for translating the internal NIF mesh structure into
    something Ogre can use.

    You have to insert meshes manually into Ogre like this:

    NIFLoader::load("somemesh.nif");

    This returns a list of meshes used by the model, as well as the names of
    their parent nodes (as they pertain to the skeleton, which is optionally
    returned in the second argument if it exists).
 */
class NIFLoader
{
    static MeshPairList load(std::string name, std::string skelName, const std::string &group);

public:
    static EntityList createEntities(Ogre::Entity *parent, const std::string &bonename,
                                     Ogre::SceneNode *parentNode,
                                     const std::string &name,
                                     const std::string &group="General");

    static EntityList createEntities(Ogre::SceneNode *parent,
                                     TextKeyMap *textkeys,
                                     const std::string &name,
                                     const std::string &group="General");
};

}

#endif


