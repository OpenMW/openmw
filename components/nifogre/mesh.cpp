#include "mesh.hpp"

#include <limits>

#include <OgreMeshManager.h>
#include <OgreMesh.h>
#include <OgreSubMesh.h>
#include <OgreBone.h>
#include <OgreHardwareBufferManager.h>
#include <OgreMaterialManager.h>
#include <OgreSkeletonManager.h>
#include <OgreRenderSystem.h>
#include <OgreRoot.h>
#include <OgreSkeleton.h>
#include <OgreKeyFrame.h>

#include <components/nif/node.hpp>
#include <components/nifcache/nifcache.hpp>
#include <components/misc/stringops.hpp>

#include "material.hpp"

namespace NifOgre
{

// Helper class that computes the bounding box and of a mesh
class BoundsFinder
{
    struct MaxMinFinder
    {
        float max, min;

        MaxMinFinder()
        {
            min = std::numeric_limits<float>::infinity();
            max = -min;
        }

        void add(float f)
        {
            if (f > max) max = f;
            if (f < min) min = f;
        }

        // Return Max(max**2, min**2)
        float getMaxSquared()
        {
            float m1 = max*max;
            float m2 = min*min;
            if (m1 >= m2) return m1;
            return m2;
        }
    };

    MaxMinFinder X, Y, Z;

public:
    // Add 'verts' vertices to the calculation. The 'data' pointer is
    // expected to point to 3*verts floats representing x,y,z for each
    // point.
    void add(float *data, int verts)
    {
        for (int i=0;i<verts;i++)
        {
            X.add(*(data++));
            Y.add(*(data++));
            Z.add(*(data++));
        }
    }

    // True if this structure has valid values
    bool isValid()
    {
        return
            minX() <= maxX() &&
            minY() <= maxY() &&
            minZ() <= maxZ();
    }

    // Compute radius
    float getRadius()
    {
        assert(isValid());

        // The radius is computed from the origin, not from the geometric
        // center of the mesh.
        return sqrt(X.getMaxSquared() + Y.getMaxSquared() + Z.getMaxSquared());
    }

    float minX() {
        return X.min;
    }
    float maxX() {
        return X.max;
    }
    float minY() {
        return Y.min;
    }
    float maxY() {
        return Y.max;
    }
    float minZ() {
        return Z.min;
    }
    float maxZ() {
        return Z.max;
    }
};


NIFMeshLoader::LoaderMap NIFMeshLoader::sLoaders;

void NIFMeshLoader::createSubMesh(Ogre::Mesh *mesh, const Nif::NiTriShape *shape)
{
    const Nif::NiTriShapeData *data = shape->data.getPtr();
    const Nif::NiSkinInstance *skin = (shape->skin.empty() ? NULL : shape->skin.getPtr());
    std::vector<Ogre::Vector3> srcVerts = data->vertices;
    std::vector<Ogre::Vector3> srcNorms = data->normals;
    Ogre::HardwareBuffer::Usage vertUsage = Ogre::HardwareBuffer::HBU_STATIC;
    bool vertShadowBuffer = false;

    if(skin != NULL)
    {
        vertUsage = Ogre::HardwareBuffer::HBU_DYNAMIC_WRITE_ONLY;
        vertShadowBuffer = true;

        // Only set a skeleton when skinning. Unskinned meshes with a skeleton will be
        // explicitly attached later.
        mesh->setSkeletonName(mName);

        // Convert vertices and normals to bone space from bind position. It would be
        // better to transform the bones into bind position, but there doesn't seem to
        // be a reliable way to do that.
        std::vector<Ogre::Vector3> newVerts(srcVerts.size(), Ogre::Vector3(0.0f));
        std::vector<Ogre::Vector3> newNorms(srcNorms.size(), Ogre::Vector3(0.0f));

        const Nif::NiSkinData *data = skin->data.getPtr();
        const Nif::NodeList &bones = skin->bones;
        for(size_t b = 0;b < bones.length();b++)
        {
            Ogre::Matrix4 mat;
            mat.makeTransform(data->bones[b].trafo.trans, Ogre::Vector3(data->bones[b].trafo.scale),
                              Ogre::Quaternion(data->bones[b].trafo.rotation));
            mat = bones[b]->getWorldTransform() * mat;

            const std::vector<Nif::NiSkinData::VertWeight> &weights = data->bones[b].weights;
            for(size_t i = 0;i < weights.size();i++)
            {
                size_t index = weights[i].vertex;
                float weight = weights[i].weight;

                newVerts.at(index) += (mat*srcVerts[index]) * weight;
                if(newNorms.size() > index)
                {
                    Ogre::Vector4 vec4(srcNorms[index][0], srcNorms[index][1], srcNorms[index][2], 0.0f);
                    vec4 = mat*vec4 * weight;
                    newNorms[index] += Ogre::Vector3(&vec4[0]);
                }
            }
        }

        srcVerts = newVerts;
        srcNorms = newNorms;
    }
    else
    {
        Ogre::SkeletonManager *skelMgr = Ogre::SkeletonManager::getSingletonPtr();
        if(skelMgr->getByName(mName).isNull())
        {
            // No skinning and no skeleton, so just transform the vertices and
            // normals into position.
            Ogre::Matrix4 mat4 = shape->getWorldTransform();
            for(size_t i = 0;i < srcVerts.size();i++)
            {
                Ogre::Vector4 vec4(srcVerts[i].x, srcVerts[i].y, srcVerts[i].z, 1.0f);
                vec4 = mat4*vec4;
                srcVerts[i] = Ogre::Vector3(&vec4[0]);
            }
            for(size_t i = 0;i < srcNorms.size();i++)
            {
                Ogre::Vector4 vec4(srcNorms[i].x, srcNorms[i].y, srcNorms[i].z, 0.0f);
                vec4 = mat4*vec4;
                srcNorms[i] = Ogre::Vector3(&vec4[0]);
            }
        }
    }

    // Set the bounding box first
    BoundsFinder bounds;
    bounds.add(&srcVerts[0][0], srcVerts.size());
    if(!bounds.isValid())
    {
        float v[3] = { 0.0f, 0.0f, 0.0f };
        bounds.add(&v[0], 1);
    }

    mesh->_setBounds(Ogre::AxisAlignedBox(bounds.minX()-0.5f, bounds.minY()-0.5f, bounds.minZ()-0.5f,
                                          bounds.maxX()+0.5f, bounds.maxY()+0.5f, bounds.maxZ()+0.5f));
    mesh->_setBoundingSphereRadius(bounds.getRadius());

    // This function is just one long stream of Ogre-barf, but it works
    // great.
    Ogre::HardwareBufferManager *hwBufMgr = Ogre::HardwareBufferManager::getSingletonPtr();
    Ogre::HardwareVertexBufferSharedPtr vbuf;
    Ogre::HardwareIndexBufferSharedPtr ibuf;
    Ogre::VertexBufferBinding *bind;
    Ogre::VertexDeclaration *decl;
    int nextBuf = 0;

    Ogre::SubMesh *sub = mesh->createSubMesh();

    // Add vertices
    sub->useSharedVertices = false;
    sub->vertexData = new Ogre::VertexData();
    sub->vertexData->vertexStart = 0;
    sub->vertexData->vertexCount = srcVerts.size();

    decl = sub->vertexData->vertexDeclaration;
    bind = sub->vertexData->vertexBufferBinding;
    if(srcVerts.size())
    {
        vbuf = hwBufMgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3),
                                            srcVerts.size(), vertUsage, vertShadowBuffer);
        vbuf->writeData(0, vbuf->getSizeInBytes(), &srcVerts[0][0], true);

        decl->addElement(nextBuf, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
        bind->setBinding(nextBuf++, vbuf);
    }

    // Vertex normals
    if(srcNorms.size())
    {
        vbuf = hwBufMgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3),
                                            srcNorms.size(), vertUsage, vertShadowBuffer);
        vbuf->writeData(0, vbuf->getSizeInBytes(), &srcNorms[0][0], true);

        decl->addElement(nextBuf, 0, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
        bind->setBinding(nextBuf++, vbuf);
    }

    // Vertex colors
    const std::vector<Ogre::Vector4> &colors = data->colors;
    if(colors.size())
    {
        Ogre::RenderSystem *rs = Ogre::Root::getSingleton().getRenderSystem();
        std::vector<Ogre::RGBA> colorsRGB(colors.size());
        for(size_t i = 0;i < colorsRGB.size();i++)
        {
            Ogre::ColourValue clr(colors[i][0], colors[i][1], colors[i][2], colors[i][3]);
            rs->convertColourValue(clr, &colorsRGB[i]);
        }
        vbuf = hwBufMgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_COLOUR),
                                            colorsRGB.size(), Ogre::HardwareBuffer::HBU_STATIC);
        vbuf->writeData(0, vbuf->getSizeInBytes(), &colorsRGB[0], true);
        decl->addElement(nextBuf, 0, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE);
        bind->setBinding(nextBuf++, vbuf);
    }

    // Texture UV coordinates
    size_t numUVs = data->uvlist.size();
    if (numUVs)
    {
        size_t elemSize = Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2);

        for(size_t i = 0; i < numUVs; i++)
            decl->addElement(nextBuf, elemSize*i, Ogre::VET_FLOAT2, Ogre::VES_TEXTURE_COORDINATES, i);

        vbuf = hwBufMgr->createVertexBuffer(decl->getVertexSize(nextBuf), srcVerts.size(),
                                            Ogre::HardwareBuffer::HBU_STATIC);

        std::vector<Ogre::Vector2> allUVs;
        allUVs.reserve(srcVerts.size()*numUVs);
        for (size_t vert = 0; vert<srcVerts.size(); ++vert)
            for(size_t i = 0; i < numUVs; i++)
                allUVs.push_back(data->uvlist[i][vert]);

        vbuf->writeData(0, elemSize*srcVerts.size()*numUVs, &allUVs[0], true);

        bind->setBinding(nextBuf++, vbuf);
    }

    // Triangle faces
    const std::vector<short> &srcIdx = data->triangles;
    if(srcIdx.size())
    {
        ibuf = hwBufMgr->createIndexBuffer(Ogre::HardwareIndexBuffer::IT_16BIT, srcIdx.size(),
                                           Ogre::HardwareBuffer::HBU_STATIC);
        ibuf->writeData(0, ibuf->getSizeInBytes(), &srcIdx[0], true);
        sub->indexData->indexBuffer = ibuf;
        sub->indexData->indexCount = srcIdx.size();
        sub->indexData->indexStart = 0;
    }

    // Assign bone weights for this TriShape
    if(skin != NULL)
    {
        Ogre::SkeletonPtr skel = Ogre::SkeletonManager::getSingleton().getByName(mName);

        const Nif::NiSkinData *data = skin->data.getPtr();
        const Nif::NodeList &bones = skin->bones;
        for(size_t i = 0;i < bones.length();i++)
        {
            Ogre::VertexBoneAssignment boneInf;
            boneInf.boneIndex = skel->getBone(bones[i]->name)->getHandle();

            const std::vector<Nif::NiSkinData::VertWeight> &weights = data->bones[i].weights;
            for(size_t j = 0;j < weights.size();j++)
            {
                boneInf.vertexIndex = weights[j].vertex;
                boneInf.weight = weights[j].weight;
                sub->addBoneAssignment(boneInf);
            }
        }
    }

    const Nif::NiTexturingProperty *texprop = NULL;
    const Nif::NiMaterialProperty *matprop = NULL;
    const Nif::NiAlphaProperty *alphaprop = NULL;
    const Nif::NiVertexColorProperty *vertprop = NULL;
    const Nif::NiZBufferProperty *zprop = NULL;
    const Nif::NiSpecularProperty *specprop = NULL;
    const Nif::NiWireframeProperty *wireprop = NULL;
    const Nif::NiStencilProperty *stencilprop = NULL;
    bool needTangents = false;

    shape->getProperties(texprop, matprop, alphaprop, vertprop, zprop, specprop, wireprop, stencilprop);
    std::string matname = NIFMaterialLoader::getMaterial(data, mesh->getName(), mGroup,
                                                         texprop, matprop, alphaprop,
                                                         vertprop, zprop, specprop,
                                                         wireprop, stencilprop, needTangents);
    if(matname.length() > 0)
        sub->setMaterialName(matname);

    // build tangents if the material needs them
    if (needTangents)
    {
        unsigned short src,dest;
        if (!mesh->suggestTangentVectorBuildParams(Ogre::VES_TANGENT, src,dest))
            mesh->buildTangentVectors(Ogre::VES_TANGENT, src,dest);
    }


    if(!shape->controller.empty())
    {
        Nif::ControllerPtr ctrl = shape->controller;
        do {
            // Load GeomMorpherController into an Ogre::Pose and Animation
            if(ctrl->recType == Nif::RC_NiGeomMorpherController && ctrl->flags & Nif::NiNode::ControllerFlag_Active)
            {
                const Nif::NiGeomMorpherController *geom =
                        static_cast<const Nif::NiGeomMorpherController*>(ctrl.getPtr());

                const std::vector<Nif::NiMorphData::MorphData>& morphs = geom->data.getPtr()->mMorphs;
                // Note we are not interested in morph 0, which just contains the original vertices
                for (unsigned int i = 1; i < morphs.size(); ++i)
                {
                    Ogre::Pose* pose = mesh->createPose(i);
                    const Nif::NiMorphData::MorphData& data = morphs[i];
                    for (unsigned int v = 0; v < data.mVertices.size(); ++v)
                        pose->addVertex(v, data.mVertices[v]);

                    Ogre::String animationID = Ogre::StringConverter::toString(ctrl->recIndex)
                            + "_" + Ogre::StringConverter::toString(i);
                    Ogre::VertexAnimationTrack* track =
                            mesh->createAnimation(animationID, 0)
                            ->createVertexTrack(1, Ogre::VAT_POSE);
                    Ogre::VertexPoseKeyFrame* keyframe = track->createVertexPoseKeyFrame(0);
                    keyframe->addPoseReference(i-1, 1);
                }

                break;
            }
        } while(!(ctrl=ctrl->next).empty());
    }
}


NIFMeshLoader::NIFMeshLoader(const std::string &name, const std::string &group, size_t idx)
  : mName(name), mGroup(group), mShapeIndex(idx)
{
}

void NIFMeshLoader::loadResource(Ogre::Resource *resource)
{
    Ogre::Mesh *mesh = static_cast<Ogre::Mesh*>(resource);
    OgreAssert(mesh, "Attempting to load a mesh into a non-mesh resource!");

    Nif::NIFFilePtr nif = Nif::Cache::getInstance().load(mName);
    if(mShapeIndex >= nif->numRecords())
    {
        Ogre::SkeletonManager *skelMgr = Ogre::SkeletonManager::getSingletonPtr();
        if(!skelMgr->getByName(mName).isNull())
            mesh->setSkeletonName(mName);
        return;
    }

    const Nif::Record *record = nif->getRecord(mShapeIndex);
    createSubMesh(mesh, static_cast<const Nif::NiTriShape*>(record));
}


void NIFMeshLoader::createMesh(const std::string &name, const std::string &fullname, const std::string &group, size_t idx)
{
    NIFMeshLoader::LoaderMap::iterator loader;
    loader = sLoaders.insert(std::make_pair(fullname, NIFMeshLoader(name, group, idx))).first;

    Ogre::MeshManager &meshMgr = Ogre::MeshManager::getSingleton();
    Ogre::MeshPtr mesh = meshMgr.createManual(fullname, group, &loader->second);
    mesh->setAutoBuildEdgeLists(false);
}

}
