#include "world.hpp"

#include <OgreAxisAlignedBox.h>
#include <OgreCamera.h>
#include <OgreHardwareBufferManager.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreRoot.h>

#include <components/loadinglistener/loadinglistener.hpp>

#include "storage.hpp"
#include "quadtreenode.hpp"

namespace
{

    bool isPowerOfTwo(int x)
    {
        return ( (x > 0) && ((x & (x - 1)) == 0) );
    }

    int nextPowerOfTwo (int v)
    {
        if (isPowerOfTwo(v)) return v;
        int depth=0;
        while(v)
        {
            v >>= 1;
            depth++;
        }
        return 1 << depth;
    }

    Terrain::QuadTreeNode* findNode (const Ogre::Vector2& center, Terrain::QuadTreeNode* node)
    {
        if (center == node->getCenter())
            return node;

        if (center.x > node->getCenter().x && center.y > node->getCenter().y)
            return findNode(center, node->getChild(Terrain::NE));
        else if (center.x > node->getCenter().x && center.y < node->getCenter().y)
            return findNode(center, node->getChild(Terrain::SE));
        else if (center.x < node->getCenter().x && center.y > node->getCenter().y)
            return findNode(center, node->getChild(Terrain::NW));
        else //if (center.x < node->getCenter().x && center.y < node->getCenter().y)
            return findNode(center, node->getChild(Terrain::SW));
    }

}

namespace Terrain
{

    World::World(Loading::Listener* loadingListener, Ogre::SceneManager* sceneMgr,
                     Storage* storage, int visibilityFlags, bool distantLand, bool shaders)
        : mStorage(storage)
        , mMinBatchSize(1)
        , mMaxBatchSize(64)
        , mSceneMgr(sceneMgr)
        , mVisibilityFlags(visibilityFlags)
        , mDistantLand(distantLand)
        , mShaders(shaders)
        , mVisible(true)
        , mLoadingListener(loadingListener)
        , mMaxX(0)
        , mMinX(0)
        , mMaxY(0)
        , mMinY(0)
    {
        loadingListener->setLabel("Creating terrain");
        loadingListener->indicateProgress();

        mCompositeMapSceneMgr = Ogre::Root::getSingleton().createSceneManager(Ogre::ST_GENERIC);

        Ogre::Camera* compositeMapCam = mCompositeMapSceneMgr->createCamera("a");
        mCompositeMapRenderTexture = Ogre::TextureManager::getSingleton().createManual(
                    "terrain/comp/rt", Ogre::ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,
            Ogre::TEX_TYPE_2D, 128, 128, 0, Ogre::PF_A8B8G8R8, Ogre::TU_RENDERTARGET);
        mCompositeMapRenderTarget = mCompositeMapRenderTexture->getBuffer()->getRenderTarget();
        mCompositeMapRenderTarget->setAutoUpdated(false);
        mCompositeMapRenderTarget->addViewport(compositeMapCam);

        storage->getBounds(mMinX, mMaxX, mMinY, mMaxY);

        int origSizeX = mMaxX-mMinX;
        int origSizeY = mMaxY-mMinY;

        // Dividing a quad tree only works well for powers of two, so round up to the nearest one
        int size = nextPowerOfTwo(std::max(origSizeX, origSizeY));

        // Adjust the center according to the new size
        float centerX = (mMinX+mMaxX)/2.f + (size-origSizeX)/2.f;
        float centerY = (mMinY+mMaxY)/2.f + (size-origSizeY)/2.f;

        mRootSceneNode = mSceneMgr->getRootSceneNode()->createChildSceneNode();

        mRootNode = new QuadTreeNode(this, Root, size, Ogre::Vector2(centerX, centerY), NULL);
        buildQuadTree(mRootNode);
        loadingListener->indicateProgress();
        mRootNode->initAabb();
        loadingListener->indicateProgress();
        mRootNode->initNeighbours();
        loadingListener->indicateProgress();
    }

    World::~World()
    {
        delete mRootNode;
        delete mStorage;
    }

    void World::buildQuadTree(QuadTreeNode *node)
    {
        float halfSize = node->getSize()/2.f;

        if (node->getSize() <= mMinBatchSize)
        {
            // We arrived at a leaf
            float minZ,maxZ;
            Ogre::Vector2 center = node->getCenter();
            float cellWorldSize = getStorage()->getCellWorldSize();
            if (mStorage->getMinMaxHeights(node->getSize(), center, minZ, maxZ))
                node->setBoundingBox(Ogre::AxisAlignedBox(Ogre::Vector3(-halfSize*cellWorldSize, -halfSize*cellWorldSize, minZ),
                                                          Ogre::Vector3(halfSize*cellWorldSize, halfSize*cellWorldSize, maxZ)));
            else
                node->markAsDummy(); // no data available for this node, skip it
            return;
        }

        if (node->getCenter().x - halfSize > mMaxX
                || node->getCenter().x + halfSize < mMinX
                || node->getCenter().y - halfSize > mMaxY
                || node->getCenter().y + halfSize < mMinY )
            // Out of bounds of the actual terrain - this will happen because
            // we rounded the size up to the next power of two
        {
            node->markAsDummy();
            return;
        }

        // Not a leaf, create its children
        node->createChild(SW, halfSize, node->getCenter() - halfSize/2.f);
        node->createChild(SE, halfSize, node->getCenter() + Ogre::Vector2(halfSize/2.f, -halfSize/2.f));
        node->createChild(NW, halfSize, node->getCenter() + Ogre::Vector2(-halfSize/2.f, halfSize/2.f));
        node->createChild(NE, halfSize, node->getCenter() + halfSize/2.f);
        buildQuadTree(node->getChild(SW));
        buildQuadTree(node->getChild(SE));
        buildQuadTree(node->getChild(NW));
        buildQuadTree(node->getChild(NE));

        // if all children are dummy, we are also dummy
        for (int i=0; i<4; ++i)
        {
            if (!node->getChild((ChildDirection)i)->isDummy())
                return;
        }
        node->markAsDummy();
    }

    void World::update(const Ogre::Vector3& cameraPos)
    {
        if (!mVisible)
            return;
        mRootNode->update(cameraPos, mLoadingListener);
        mRootNode->updateIndexBuffers();
    }

    Ogre::AxisAlignedBox World::getWorldBoundingBox (const Ogre::Vector2& center)
    {
        if (center.x > mMaxX
                 || center.x < mMinX
                || center.y > mMaxY
                || center.y < mMinY)
            return Ogre::AxisAlignedBox::BOX_NULL;
        QuadTreeNode* node = findNode(center, mRootNode);
        Ogre::AxisAlignedBox box = node->getBoundingBox();
        float cellWorldSize = getStorage()->getCellWorldSize();
        box.setExtents(box.getMinimum() + Ogre::Vector3(center.x, center.y, 0) * cellWorldSize,
                       box.getMaximum() + Ogre::Vector3(center.x, center.y, 0) * cellWorldSize);
        return box;
    }

    Ogre::HardwareVertexBufferSharedPtr World::getVertexBuffer(int numVertsOneSide)
    {
        if (mUvBufferMap.find(numVertsOneSide) != mUvBufferMap.end())
        {
            return mUvBufferMap[numVertsOneSide];
        }

        int vertexCount = numVertsOneSide * numVertsOneSide;

        std::vector<float> uvs;
        uvs.reserve(vertexCount*2);

        for (int col = 0; col < numVertsOneSide; ++col)
        {
            for (int row = 0; row < numVertsOneSide; ++row)
            {
                uvs.push_back(col / static_cast<float>(numVertsOneSide-1)); // U
                uvs.push_back(row / static_cast<float>(numVertsOneSide-1)); // V
            }
        }

        Ogre::HardwareBufferManager* mgr = Ogre::HardwareBufferManager::getSingletonPtr();
        Ogre::HardwareVertexBufferSharedPtr buffer = mgr->createVertexBuffer(
                    Ogre::VertexElement::getTypeSize(Ogre::VET_FLOAT2),
                                                vertexCount, Ogre::HardwareBuffer::HBU_STATIC);

        buffer->writeData(0, buffer->getSizeInBytes(), &uvs[0], true);

        mUvBufferMap[numVertsOneSide] = buffer;
        return buffer;
    }

    Ogre::HardwareIndexBufferSharedPtr World::getIndexBuffer(int flags, size_t& numIndices)
    {
        unsigned int verts = mStorage->getCellVertices();

        if (mIndexBufferMap.find(flags) != mIndexBufferMap.end())
        {
            numIndices = mIndexBufferMap[flags]->getNumIndexes();
            return mIndexBufferMap[flags];
        }

        // LOD level n means every 2^n-th vertex is kept
        size_t lodLevel = (flags >> (4*4));

        size_t lodDeltas[4];
        for (int i=0; i<4; ++i)
            lodDeltas[i] = (flags >> (4*i)) & (0xf);

        bool anyDeltas = (lodDeltas[North] || lodDeltas[South] || lodDeltas[West] || lodDeltas[East]);

        size_t increment = 1 << lodLevel;
        assert(increment < verts);
        std::vector<short> indices;
        indices.reserve((verts-1)*(verts-1)*2*3 / increment);

        size_t rowStart = 0, colStart = 0, rowEnd = verts-1, colEnd = verts-1;
        // If any edge needs stitching we'll skip all edges at this point,
        // mainly because stitching one edge would have an effect on corners and on the adjacent edges
        if (anyDeltas)
        {
            colStart += increment;
            colEnd -= increment;
            rowEnd -= increment;
            rowStart += increment;
        }
        for (size_t row = rowStart; row < rowEnd; row += increment)
        {
            for (size_t col = colStart; col < colEnd; col += increment)
            {
                indices.push_back(verts*col+row);
                indices.push_back(verts*(col+increment)+row+increment);
                indices.push_back(verts*col+row+increment);

                indices.push_back(verts*col+row);
                indices.push_back(verts*(col+increment)+row);
                indices.push_back(verts*(col+increment)+row+increment);
            }
        }

        size_t innerStep = increment;
        if (anyDeltas)
        {
            // Now configure LOD transitions at the edges - this is pretty tedious,
            // and some very long and boring code, but it works great

            // South
            size_t row = 0;
            size_t outerStep = 1 << (lodDeltas[South] + lodLevel);
            for (size_t col = 0; col < verts-1; col += outerStep)
            {
                indices.push_back(verts*col+row);
                indices.push_back(verts*(col+outerStep)+row);
                // Make sure not to touch the right edge
                if (col+outerStep == verts-1)
                    indices.push_back(verts*(col+outerStep-innerStep)+row+innerStep);
                else
                    indices.push_back(verts*(col+outerStep)+row+innerStep);

                for (size_t i = 0; i < outerStep; i += innerStep)
                {
                    // Make sure not to touch the left or right edges
                    if (col+i == 0 || col+i == verts-1-innerStep)
                        continue;
                    indices.push_back(verts*(col)+row);
                    indices.push_back(verts*(col+i+innerStep)+row+innerStep);
                    indices.push_back(verts*(col+i)+row+innerStep);
                }
            }

            // North
            row = verts-1;
            outerStep = 1 << (lodDeltas[North] + lodLevel);
            for (size_t col = 0; col < verts-1; col += outerStep)
            {
                indices.push_back(verts*(col+outerStep)+row);
                indices.push_back(verts*col+row);
                // Make sure not to touch the left edge
                if (col == 0)
                    indices.push_back(verts*(col+innerStep)+row-innerStep);
                else
                    indices.push_back(verts*col+row-innerStep);

                for (size_t i = 0; i < outerStep; i += innerStep)
                {
                    // Make sure not to touch the left or right edges
                    if (col+i == 0 || col+i == verts-1-innerStep)
                        continue;
                    indices.push_back(verts*(col+i)+row-innerStep);
                    indices.push_back(verts*(col+i+innerStep)+row-innerStep);
                    indices.push_back(verts*(col+outerStep)+row);
                }
            }

            // West
            size_t col = 0;
            outerStep = 1 << (lodDeltas[West] + lodLevel);
            for (size_t row = 0; row < verts-1; row += outerStep)
            {
                indices.push_back(verts*col+row+outerStep);
                indices.push_back(verts*col+row);
                // Make sure not to touch the top edge
                if (row+outerStep == verts-1)
                    indices.push_back(verts*(col+innerStep)+row+outerStep-innerStep);
                else
                    indices.push_back(verts*(col+innerStep)+row+outerStep);

                for (size_t i = 0; i < outerStep; i += innerStep)
                {
                    // Make sure not to touch the top or bottom edges
                    if (row+i == 0 || row+i == verts-1-innerStep)
                        continue;
                    indices.push_back(verts*col+row);
                    indices.push_back(verts*(col+innerStep)+row+i);
                    indices.push_back(verts*(col+innerStep)+row+i+innerStep);
                }
            }

            // East
            col = verts-1;
            outerStep = 1 << (lodDeltas[East] + lodLevel);
            for (size_t row = 0; row < verts-1; row += outerStep)
            {
                indices.push_back(verts*col+row);
                indices.push_back(verts*col+row+outerStep);
                // Make sure not to touch the bottom edge
                if (row == 0)
                    indices.push_back(verts*(col-innerStep)+row+innerStep);
                else
                    indices.push_back(verts*(col-innerStep)+row);

                for (size_t i = 0; i < outerStep; i += innerStep)
                {
                    // Make sure not to touch the top or bottom edges
                    if (row+i == 0 || row+i == verts-1-innerStep)
                        continue;
                    indices.push_back(verts*col+row+outerStep);
                    indices.push_back(verts*(col-innerStep)+row+i+innerStep);
                    indices.push_back(verts*(col-innerStep)+row+i);
                }
            }
        }



        numIndices = indices.size();

        Ogre::HardwareBufferManager* mgr = Ogre::HardwareBufferManager::getSingletonPtr();
        Ogre::HardwareIndexBufferSharedPtr buffer = mgr->createIndexBuffer(Ogre::HardwareIndexBuffer::IT_16BIT,
                                                                           numIndices, Ogre::HardwareBuffer::HBU_STATIC);
        buffer->writeData(0, buffer->getSizeInBytes(), &indices[0], true);
        mIndexBufferMap[flags] = buffer;
        return buffer;
    }

    void World::renderCompositeMap(Ogre::TexturePtr target)
    {
        mCompositeMapRenderTarget->update();
        target->getBuffer()->blit(mCompositeMapRenderTexture->getBuffer());
    }

    void World::clearCompositeMapSceneManager()
    {
        mCompositeMapSceneMgr->destroyAllManualObjects();
        mCompositeMapSceneMgr->clearScene();
    }

    float World::getHeightAt(const Ogre::Vector3 &worldPos)
    {
        return mStorage->getHeightAt(worldPos);
    }

    void World::applyMaterials(bool shadows, bool splitShadows)
    {
        mShadows = shadows;
        mSplitShadows = splitShadows;
        mRootNode->applyMaterials();
    }

    void World::setVisible(bool visible)
    {
        if (visible && !mVisible)
            mSceneMgr->getRootSceneNode()->addChild(mRootSceneNode);
        else if (!visible && mVisible)
            mSceneMgr->getRootSceneNode()->removeChild(mRootSceneNode);

        mVisible = visible;
    }

    bool World::getVisible()
    {
        return mVisible;
    }


}
