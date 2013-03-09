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

#ifndef OPENMW_COMPONENTS_NIFOGRE_OGRENIFLOADER_HPP
#define OPENMW_COMPONENTS_NIFOGRE_OGRENIFLOADER_HPP

#include <OgreResource.h>
#include <OgreMesh.h>
#include <OgreSkeleton.h>

#include <vector>
#include <string>


// FIXME: This namespace really doesn't do anything Nif-specific. Any supportable
// model format should go through this.
namespace NifOgre
{

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

    MeshInfo(const std::string &name, const std::string &target)
      : mMeshName(name), mTargetNode(target)
    { }
};
typedef std::vector<MeshInfo> MeshInfoList;

class Loader
{
    static MeshInfoList load(const std::string &name, const std::string &group);

public:
    static EntityList createEntities(Ogre::Entity *parent, const std::string &bonename,
                                     Ogre::SceneNode *parentNode,
                                     std::string name,
                                     const std::string &group="General");

    static EntityList createEntities(Ogre::SceneNode *parentNode,
                                     std::string name,
                                     const std::string &group="General");

    static Ogre::SkeletonPtr getSkeleton(std::string name, const std::string &group="General");
};

}

namespace std
{

// These operators allow extra data types to be stored in an Ogre::Any
// object, which can then be stored in user object bindings on the nodes

ostream& operator<<(ostream &o, const NifOgre::TextKeyMap&);

}

#endif
