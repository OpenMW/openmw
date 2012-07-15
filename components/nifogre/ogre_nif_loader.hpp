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

#include <cassert>
#include <string>
#include <boost/algorithm/string.hpp>

#include <libs/mangle/vfs/servers/ogre_vfs.hpp>
#include "../nif/nif_file.hpp"
#include "../nif/node.hpp"
#include "../nif/data.hpp"
#include "../nif/property.hpp"
#include "../nif/controller.hpp"
#include "../nif/extra.hpp"
#include <libs/platform/strings.h>

#include <vector>
#include <map>
// For warning messages
#include <limits>
using namespace boost::algorithm;


class BoundsFinder;

struct ciLessBoost : std::binary_function<std::string, std::string, bool>
{
    bool operator() (const std::string & s1, const std::string & s2) const {
                                               //case insensitive version of is_less
        return lexicographical_compare(s1, s2, is_iless());
    }
};

namespace Nif
{
    class Node;
    class Transformation;
    class NiTriShape;
}


namespace NifOgre
{

/** This holds a list of meshes along with the names of their parent nodes
 */
typedef std::vector< std::pair<Ogre::MeshPtr,std::string> > MeshPairList;


/** Manual resource loader for NIF meshes. This is the main class
    responsible for translating the internal NIF mesh structure into
    something Ogre can use.

    You have to insert meshes manually into Ogre like this:

    NIFLoader::load("somemesh.nif");

    This returns a list of meshes used by the model, as well as the names of
    their parent nodes (as they pertain to the skeleton, which is optionally
    returned in the second argument if it exists).
 */
class NIFLoader : Ogre::ManualResourceLoader
{
public:
    virtual void loadResource(Ogre::Resource *resource);

    static MeshPairList load(const std::string &name,
                             Ogre::SkeletonPtr *skel=NULL,
                             const std::string &group="General");

private:
    std::string mName;
    std::string mShapeName;

    static void warn(const std::string &msg);
    static void fail(const std::string &msg);

    static void createMeshes(const std::string &name, const std::string &group, Nif::Node *node, MeshPairList &meshes, int flags=0);

    typedef std::map<std::string,NIFLoader,ciLessBoost> LoaderMap;
    static LoaderMap sLoaders;
};

}

#endif
