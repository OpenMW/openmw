#include "chunk.hpp"

#include <OgreSceneNode.h>
#include <OgreHardwareBufferManager.h>
#include <OgreRenderQueue.h>

#include "world.hpp" // FIXME: for LoadResponseData, move to backgroundloader.hpp

namespace Terrain
{

    Chunk::Chunk(Ogre::HardwareVertexBufferSharedPtr uvBuffer, const Ogre::AxisAlignedBox& bounds, const LoadResponseData& data)
        : mBounds(bounds)
    {
        mVertexData = OGRE_NEW Ogre::VertexData;
        mVertexData->vertexStart = 0;
        mVertexData->vertexCount = data.mPositions.size()/3;

        // Set up the vertex declaration, which specifies the info for each vertex (normals, colors, UVs, etc)
        Ogre::VertexDeclaration* vertexDecl = mVertexData->vertexDeclaration;

        Ogre::HardwareBufferManager* mgr = Ogre::HardwareBufferManager::getSingletonPtr();
        size_t nextBuffer = 0;

        // Positions
        vertexDecl->addElement(nextBuffer++, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
        mVertexBuffer = mgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3),
                                                mVertexData->vertexCount, Ogre::HardwareBuffer::HBU_STATIC);

        // Normals
        vertexDecl->addElement(nextBuffer++, 0, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
        mNormalBuffer = mgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3),
                                                mVertexData->vertexCount, Ogre::HardwareBuffer::HBU_STATIC);


        // UV texture coordinates
        vertexDecl->addElement(nextBuffer++, 0, Ogre::VET_FLOAT2,
                               Ogre::VES_TEXTURE_COORDINATES, 0);

        // Colours
        vertexDecl->addElement(nextBuffer++, 0, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE);
        mColourBuffer = mgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_COLOUR),
                                                mVertexData->vertexCount, Ogre::HardwareBuffer::HBU_STATIC);

        mVertexBuffer->writeData(0, mVertexBuffer->getSizeInBytes(), &data.mPositions[0], true);
        mNormalBuffer->writeData(0, mNormalBuffer->getSizeInBytes(), &data.mNormals[0], true);
        mColourBuffer->writeData(0, mColourBuffer->getSizeInBytes(), &data.mColours[0], true);

        mVertexData->vertexBufferBinding->setBinding(0, mVertexBuffer);
        mVertexData->vertexBufferBinding->setBinding(1, mNormalBuffer);
        mVertexData->vertexBufferBinding->setBinding(2, uvBuffer);
        mVertexData->vertexBufferBinding->setBinding(3, mColourBuffer);

        mIndexData = OGRE_NEW Ogre::IndexData();
        mIndexData->indexStart = 0;
    }

    void Chunk::setIndexBuffer(Ogre::HardwareIndexBufferSharedPtr buffer)
    {
        mIndexData->indexBuffer = buffer;
        mIndexData->indexCount = buffer->getNumIndexes();
    }

    Chunk::~Chunk()
    {
        OGRE_DELETE mVertexData;
        OGRE_DELETE mIndexData;
    }

    void Chunk::setMaterial(const Ogre::MaterialPtr &material)
    {
        mMaterial = material;
    }

    const Ogre::AxisAlignedBox& Chunk::getBoundingBox(void) const
    {
        return mBounds;
    }

    Ogre::Real Chunk::getBoundingRadius(void) const
    {
        return mBounds.getHalfSize().length();
    }

    void Chunk::_updateRenderQueue(Ogre::RenderQueue* queue)
    {
        queue->addRenderable(this, mRenderQueueID);
    }

    void Chunk::visitRenderables(Ogre::Renderable::Visitor* visitor,
        bool debugRenderables)
    {
        visitor->visit(this, 0, false);
    }

    const Ogre::MaterialPtr& Chunk::getMaterial(void) const
    {
        return mMaterial;
    }

    void Chunk::getRenderOperation(Ogre::RenderOperation& op)
    {
        assert (!mIndexData->indexBuffer.isNull() && "Trying to render, but no index buffer set!");
        op.useIndexes = true;
        op.operationType = Ogre::RenderOperation::OT_TRIANGLE_LIST;
        op.vertexData = mVertexData;
        op.indexData = mIndexData;
    }

    void Chunk::getWorldTransforms(Ogre::Matrix4* xform) const
    {
        *xform = getParentSceneNode()->_getFullTransform();
    }

    Ogre::Real Chunk::getSquaredViewDepth(const Ogre::Camera* cam) const
    {
        return getParentSceneNode()->getSquaredViewDepth(cam);
    }

    const Ogre::LightList& Chunk::getLights(void) const
    {
        return queryLights();
    }

}
