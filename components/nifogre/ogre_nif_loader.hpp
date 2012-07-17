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

#include <cassert>
#include <boost/algorithm/string.hpp>

#include "../nif/node.hpp"

#include <libs/platform/strings.h>

class BoundsFinder;

struct ciLessBoost : std::binary_function<std::string, std::string, bool>
{
    bool operator() (const std::string & s1, const std::string & s2) const
    {
        //case insensitive version of is_less
        return boost::algorithm::lexicographical_compare(s1, s2, boost::algorithm::is_iless());
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
        //void insertMeshInsideBase(Ogre::Mesh* mesh);
        std::vector<Nif::NiKeyframeData>* getAnim(std::string name);
        std::vector<Nif::NiTriShapeCopy>* getShapes(std::string name);
        std::map<std::string, float>* getTextIndices(std::string name);


        void setOutputAnimFiles(bool output);
        void setVerbosePath(std::string path);

    private:

        NIFLoader() : resourceName(""), resourceGroup("General"),  flip(false), mNormaliseNormals(false),
          mFlipVertexWinding(false), mOutputAnimFiles(false), inTheSkeletonTree(false)  {}
        NIFLoader(NIFLoader& n) {}

        void calculateTransform();


        void warn(std::string msg);
        void fail(std::string msg);

        void handleNode( Nif::Node *node, int flags,
                        const Nif::Transformation *trafo, BoundsFinder &bounds, Ogre::Bone *parentBone, std::vector<std::string> boneSequence);

        void handleNiTriShape(Nif::NiTriShape *shape, int flags, BoundsFinder &bounds, Nif::Transformation original, std::vector<std::string> boneSequence);

        void createOgreSubMesh(Nif::NiTriShape *shape, const Ogre::String &material, std::list<Ogre::VertexBoneAssignment> &vertexBoneAssignments);

        void createMaterial(const Ogre::String &name,
                            const Ogre::Vector3 &ambient,
                            const Ogre::Vector3 &diffuse,
                            const Ogre::Vector3 &specular,
                            const Ogre::Vector3 &emissive,
                            float glossiness, float alpha,
                            int alphaFlags, float alphaTest,
                            const Ogre::String &texName,
                            bool vertexColor);

        void findRealTexture(Ogre::String &texName);

        Ogre::String getUniqueName(const Ogre::String &input);

        //returns the skeleton name of this mesh
        std::string getSkeletonName()
        {
            return resourceName + ".skel";
        }

        std::string verbosePath;
        std::string resourceName;
        std::string resourceGroup;
        Ogre::Matrix4 mTransform;
        Ogre::AxisAlignedBox mBoundingBox;
        bool flip;
        bool mNormaliseNormals;
        bool mFlipVertexWinding;
        bool bNiTri;
        bool mOutputAnimFiles;
        std::multimap<std::string,std::string> MaterialMap;

        // pointer to the ogre mesh which is currently build
        Ogre::Mesh *mesh;
        Ogre::SkeletonPtr mSkel;
        Ogre::Vector3 vector;
        std::vector<Nif::NiTriShapeCopy> shapes;
        std::string name;
        std::string triname;
        std::vector<Nif::NiKeyframeData> allanim;

		std::map<std::string,float> textmappings;
		std::map<std::string,std::map<std::string,float>,ciLessBoost> alltextmappings;
		std::map<std::string,std::vector<Nif::NiKeyframeData>,ciLessBoost> allanimmap;
		std::map<std::string,std::vector<Nif::NiTriShapeCopy>,ciLessBoost> allshapesmap;
        std::vector<Nif::NiKeyframeData> mAnim;
		std::vector<Nif::NiTriShapeCopy> mS;
        std::vector<Ogre::SubMesh*> needBoneAssignments;
        bool inTheSkeletonTree;


};

}

#endif


