/*
 * Copyright (c) 2015 scrawl <scrawl@baseoftrash.de>
 *
 * Permission is hereby granted, free of charge, to any person obtaining a copy
 * of this software and associated documentation files (the "Software"), to deal
 * in the Software without restriction, including without limitation the rights
 * to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
 * copies of the Software, and to permit persons to whom the Software is
 * furnished to do so, subject to the following conditions:
 *
 * The above copyright notice and this permission notice shall be included in
 * all copies or substantial portions of the Software.

 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
 * IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
 * FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
 * AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
 * LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
 * OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
 * THE SOFTWARE.
 */
#include "chunk.hpp"

#include <OgreSceneNode.h>
#include <OgreHardwareBufferManager.h>
#include <OgreRenderQueue.h>
#include <OgreMaterialManager.h>
#include <OgreStringConverter.h>

#include <extern/shiny/Main/Factory.hpp>

namespace Terrain
{

    Chunk::Chunk(Ogre::HardwareVertexBufferSharedPtr uvBuffer, const Ogre::AxisAlignedBox& bounds,
                 const std::vector<float>& positions, const std::vector<float>& normals, const std::vector<Ogre::uint8>& colours)
        : mBounds(bounds)
        , mOwnMaterial(false)
    {
        mVertexData = OGRE_NEW Ogre::VertexData;
        mVertexData->vertexStart = 0;
        mVertexData->vertexCount = positions.size()/3;

        // Set up the vertex declaration, which specifies the info for each vertex (normals, colors, UVs, etc)
        Ogre::VertexDeclaration* vertexDecl = mVertexData->vertexDeclaration;

        Ogre::HardwareBufferManager* mgr = Ogre::HardwareBufferManager::getSingletonPtr();
        size_t nextBuffer = 0;

        // Positions
        vertexDecl->addElement(nextBuffer++, 0, Ogre::VET_FLOAT3, Ogre::VES_POSITION);
        Ogre::HardwareVertexBufferSharedPtr vertexBuffer = mgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3),
                                                mVertexData->vertexCount, Ogre::HardwareBuffer::HBU_STATIC);

        // Normals
        vertexDecl->addElement(nextBuffer++, 0, Ogre::VET_FLOAT3, Ogre::VES_NORMAL);
        Ogre::HardwareVertexBufferSharedPtr normalBuffer = mgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT3),
                                                mVertexData->vertexCount, Ogre::HardwareBuffer::HBU_STATIC);


        // UV texture coordinates
        vertexDecl->addElement(nextBuffer++, 0, Ogre::VET_FLOAT2,
                               Ogre::VES_TEXTURE_COORDINATES, 0);

        // Colours
        vertexDecl->addElement(nextBuffer++, 0, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE);
        Ogre::HardwareVertexBufferSharedPtr colourBuffer = mgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_COLOUR),
                                                mVertexData->vertexCount, Ogre::HardwareBuffer::HBU_STATIC);

        vertexBuffer->writeData(0, vertexBuffer->getSizeInBytes(), &positions[0], true);
        normalBuffer->writeData(0, normalBuffer->getSizeInBytes(), &normals[0], true);
        colourBuffer->writeData(0, colourBuffer->getSizeInBytes(), &colours[0], true);

        mVertexData->vertexBufferBinding->setBinding(0, vertexBuffer);
        mVertexData->vertexBufferBinding->setBinding(1, normalBuffer);
        mVertexData->vertexBufferBinding->setBinding(2, uvBuffer);
        mVertexData->vertexBufferBinding->setBinding(3, colourBuffer);

        // Assign a default material in case terrain material fails to be created
        mMaterial = Ogre::MaterialManager::getSingleton().getByName("BaseWhite");

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
        if (!mMaterial.isNull() && mOwnMaterial)
        {
#if TERRAIN_USE_SHADER
            sh::Factory::getInstance().destroyMaterialInstance(mMaterial->getName());
#endif
            Ogre::MaterialManager::getSingleton().remove(mMaterial->getName());
        }
        OGRE_DELETE mVertexData;
        OGRE_DELETE mIndexData;
    }

    void Chunk::setMaterial(const Ogre::MaterialPtr &material, bool own)
    {
        // Clean up the previous material, if we own it
        if (!mMaterial.isNull() && mOwnMaterial)
        {
#if TERRAIN_USE_SHADER
            sh::Factory::getInstance().destroyMaterialInstance(mMaterial->getName());
#endif
            Ogre::MaterialManager::getSingleton().remove(mMaterial->getName());
        }

        mMaterial = material;
        mOwnMaterial = own;
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
        assert(!mMaterial.isNull() && "Trying to render, but no material set!");
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
