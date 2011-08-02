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
#include <string>


class BoundsFinder;

namespace Nif
{
    class Node;
    class Transformation;
    class NiTriShape;
    class Vector;
    class Matrix;
}

namespace Mangle
{
    namespace VFS
    {
        class OgreVFS;
    }
}

namespace NifOgre
{

/** Manual resource loader for NIF meshes. This is the main class
    responsible for translating the internal NIF mesh structure into
    something Ogre can use. Later it will also handle the insertion of
    collision meshes into Bullet / OgreBullet.

    You have to insert meshes manually into Ogre like this:

    NIFLoader::load("somemesh.nif");

    Afterwards, you can use the mesh name "somemesh.nif" normally to
    create entities and so on. The mesh isn't loaded from disk until
    OGRE needs it for rendering. Thus the above load() command is not
    very resource intensive, and can safely be done for a large number
    of meshes at load time.
 */
class NIFLoader : Ogre::ManualResourceLoader
{
    public:
        static int numberOfMeshes;
        static NIFLoader& getSingleton();
        static NIFLoader* getSingletonPtr();

        virtual void loadResource(Ogre::Resource *resource);

        static Ogre::MeshPtr load(const std::string &name,
                                    const std::string &group="General");



        Ogre::Vector3 convertVector3(const Nif::Vector& vec);
        Ogre::Quaternion convertRotation(const Nif::Matrix& rot);

    private:
        NIFLoader() : resourceGroup("General") { skincounter = 0; resourceName = "";}
        NIFLoader(NIFLoader& n) {}

        void warn(std::string msg);
        void fail(std::string msg);

        void handleNode( Nif::Node *node, int flags,
                        const Nif::Transformation *trafo, BoundsFinder &bounds, Ogre::Bone *parentBone);

        void handleNiTriShape(Nif::NiTriShape *shape, int flags, BoundsFinder &bounds);

        void createOgreSubMesh(Nif::NiTriShape *shape, const Ogre::String &material, std::list<Ogre::VertexBoneAssignment> &vertexBoneAssignments);

        void createMaterial(const Ogre::String &name,
                            const Nif::Vector &ambient,
                            const Nif::Vector &diffuse,
                            const Nif::Vector &specular,
                            const Nif::Vector &emissive,
                            float glossiness, float alpha,
                            int alphaFlags, float alphaTest,
                            const Ogre::String &texName);

        void findRealTexture(Ogre::String &texName);

        Ogre::String getUniqueName(const Ogre::String &input);

        //returns the skeleton name of this mesh
        std::string getSkeletonName()
        {
            return resourceName + ".skel";
        }

        // This is the interface to the Ogre resource system. It allows us to
        // load NIFs from BSAs, in the file system and in any other place we
        // tell Ogre to look (eg. in zip or rar files.) It's also used to
        // check for the existence of texture files, so we can exchange the
        // extension from .tga to .dds if the texture is missing.
        Mangle::VFS::OgreVFS *vfs;

        std::string resourceName;
        std::string resourceGroup;
        int skincounter;
        bool isChest;
        bool isBeast;
        bool isHands;
        bool isFeet;
        int counter;
        int numbers;
        int stack;
        std::multimap<std::string,std::string> MaterialMap;

        // pointer to the ogre mesh which is currently build
        Ogre::Mesh *mesh;
        Ogre::SkeletonPtr mSkel;
};

}

#endif
