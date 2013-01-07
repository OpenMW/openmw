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
static const char sTextKeyExtraDataID[] = "TextKeyExtraData";
struct EntityList {
    std::vector<Ogre::Entity*> mEntities;
    Ogre::Entity *mSkelBase;

    EntityList() : mSkelBase(0)
    { }
};


/* This holds a list of mesh names, the names of their parent nodes, and the offset
 * from their parent nodes. */
struct MeshInfo {
    std::string mMeshName;
    std::string mTargetNode;
    Ogre::Vector3 mPos;
    Ogre::Matrix3 mRot;
    float mScale;

    MeshInfo(const std::string &name, const std::string &target,
             const Ogre::Vector3 &pos, const Ogre::Matrix3 &rot, float scale)
      : mMeshName(name), mTargetNode(target), mPos(pos), mRot(rot), mScale(scale)
    { }
};
typedef std::vector<MeshInfo> MeshInfoList;

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
    static MeshInfoList load(const std::string &name, const std::string &skelName, const std::string &group);

public:
    static EntityList createEntities(Ogre::Entity *parent, const std::string &bonename,
                                     Ogre::SceneNode *parentNode,
                                     std::string name,
                                     const std::string &group="General");

    static EntityList createEntities(Ogre::SceneNode *parentNode,
                                     std::string name,
                                     const std::string &group="General");
};

}

#endif
