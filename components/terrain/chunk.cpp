#include "chunk.hpp"

#include <OgreSceneNode.h>
#include <OgreHardwareBufferManager.h>

#include "quadtreenode.hpp"
#include "world.hpp"
#include "storage.hpp"

namespace Terrain
{

    Chunk::Chunk(QuadTreeNode* node, short lodLevel)
        : mNode(node)
        , mVertexLod(lodLevel)
        , mAdditionalLod(0)
    {
        mVertexData = OGRE_NEW Ogre::VertexData;
        mVertexData->vertexStart = 0;

        unsigned int verts = mNode->getTerrain()->getStorage()->getCellVertices();

        // Set the total number of vertices
        size_t numVertsOneSide = mNode->getSize() * (verts-1);
        numVertsOneSide /= 1 << lodLevel;
        numVertsOneSide += 1;
        assert(numVertsOneSide == verts);
        mVertexData->vertexCount = numVertsOneSide * numVertsOneSide;

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
        Ogre::HardwareVertexBufferSharedPtr uvBuf = mNode->getTerrain()->getVertexBuffer(numVertsOneSide);

        // Colours
        vertexDecl->addElement(nextBuffer++, 0, Ogre::VET_COLOUR, Ogre::VES_DIFFUSE);
        mColourBuffer = mgr->createVertexBuffer(Ogre::VertexElement::getTypeSize(Ogre::VET_COLOUR),
                                                mVertexData->vertexCount, Ogre::HardwareBuffer::HBU_STATIC);

        mNode->getTerrain()->getStorage()->fillVertexBuffers(lodLevel, mNode->getSize(), mNode->getCenter(),
                                                             mVertexBuffer, mNormalBuffer, mColourBuffer);

        mVertexData->vertexBufferBinding->setBinding(0, mVertexBuffer);
        mVertexData->vertexBufferBinding->setBinding(1, mNormalBuffer);
        mVertexData->vertexBufferBinding->setBinding(2, uvBuf);
        mVertexData->vertexBufferBinding->setBinding(3, mColourBuffer);

        mIndexData = OGRE_NEW Ogre::IndexData();
        mIndexData->indexStart = 0;
    }



    void Chunk::updateIndexBuffer()
    {
        // Fetch a suitable index buffer (which may be shared)
        size_t ourLod = mVertexLod + mAdditionalLod;

        int flags = 0;

        for (int i=0; i<4; ++i)
        {
            QuadTreeNode* neighbour = mNode->getNeighbour((Direction)i);

            // If the neighbour isn't currently rendering itself,
            // go up until we find one. NOTE: We don't need to go down,
            // because in that case neighbour's detail would be higher than
            // our detail and the neighbour would handle stitching by itself.
            while (neighbour && !neighbour->hasChunk())
                neighbour = neighbour->getParent();

            size_t lod = 0;
            if (neighbour)
                lod = neighbour->getActualLodLevel();

            if (lod <= ourLod) // We only need to worry about neighbours less detailed than we are -
                lod = 0;         // neighbours with more detail will do the stitching themselves

            // Use 4 bits for each LOD delta
            if (lod > 0)
            {
                assert (lod - ourLod < (1 << 4));
                flags |= int(lod - ourLod) << (4*i);
            }
        }

        flags |= ((int)mAdditionalLod) << (4*4);

        size_t numIndices;
        mIndexBuffer = mNode->getTerrain()->getIndexBuffer(flags, numIndices);
        mIndexData->indexCount = numIndices;
        mIndexData->indexBuffer = mIndexBuffer;
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
        return mNode->getBoundingBox();
    }

    Ogre::Real Chunk::getBoundingRadius(void) const
    {
        return mNode->getBoundingBox().getHalfSize().length();
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
        assert (!mIndexBuffer.isNull() && "Trying to render, but no index buffer set!");
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
