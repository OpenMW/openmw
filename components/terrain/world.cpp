#include "world.hpp"

#include <OgreAxisAlignedBox.h>
#include <OgreCamera.h>
#include <OgreHardwarePixelBuffer.h>
#include <OgreTextureManager.h>
#include <OgreRenderTexture.h>
#include <OgreSceneNode.h>
#include <OgreRoot.h>
#include <OgreTimer.h> // TEMP

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

    World::World(Ogre::SceneManager* sceneMgr,
                     Storage* storage, int visibilityFlags, bool distantLand, bool shaders, Alignment align, float minBatchSize, float maxBatchSize)
        : mStorage(storage)
        , mMinBatchSize(minBatchSize)
        , mMaxBatchSize(maxBatchSize)
        , mSceneMgr(sceneMgr)
        , mVisibilityFlags(visibilityFlags)
        , mDistantLand(distantLand)
        , mShaders(shaders)
        , mVisible(true)
        , mAlign(align)
        , mMaxX(0)
        , mMinX(0)
        , mMaxY(0)
        , mMinY(0)
        , mChunksLoading(0)
        , mWorkQueueChannel(0)
        , mCache(storage->getCellVertices())
    {
#if TERRAIN_USE_SHADER == 0
        if (mShaders)
            std::cerr << "Compiled Terrain without shader support, disabling..." << std::endl;
        mShaders = false;
#endif

        mCompositeMapSceneMgr = Ogre::Root::getSingleton().createSceneManager(Ogre::ST_GENERIC);

        /// \todo make composite map size configurable
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
        //loadingListener->indicateProgress();
        mRootNode->initAabb();
        //loadingListener->indicateProgress();
        mRootNode->initNeighbours();
        //loadingListener->indicateProgress();

        Ogre::WorkQueue* wq = Ogre::Root::getSingleton().getWorkQueue();
        mWorkQueueChannel = wq->getChannel("LargeTerrain");
        wq->addRequestHandler(mWorkQueueChannel, this);
        wq->addResponseHandler(mWorkQueueChannel, this);
    }

    World::~World()
    {
        Ogre::WorkQueue* wq = Ogre::Root::getSingleton().getWorkQueue();
        wq->removeRequestHandler(mWorkQueueChannel, this);
        wq->removeResponseHandler(mWorkQueueChannel, this);

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
            {
                Ogre::AxisAlignedBox bounds(Ogre::Vector3(-halfSize*cellWorldSize, -halfSize*cellWorldSize, minZ),
                                    Ogre::Vector3(halfSize*cellWorldSize, halfSize*cellWorldSize, maxZ));
                convertBounds(bounds);
                node->setBoundingBox(bounds);
            }
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
        mRootNode->update(cameraPos);
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
        return node->getWorldBoundingBox();
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

    void World::convertPosition(float &x, float &y, float &z)
    {
        Terrain::convertPosition(mAlign, x, y, z);
    }

    void World::convertPosition(Ogre::Vector3 &pos)
    {
        convertPosition(pos.x, pos.y, pos.z);
    }

    void World::convertBounds(Ogre::AxisAlignedBox& bounds)
    {
        switch (mAlign)
        {
        case Align_XY:
            return;
        case Align_XZ:
            convertPosition(bounds.getMinimum());
            convertPosition(bounds.getMaximum());
            // Because we changed sign of Z
            std::swap(bounds.getMinimum().z, bounds.getMaximum().z);
            return;
        case Align_YZ:
            convertPosition(bounds.getMinimum());
            convertPosition(bounds.getMaximum());
            return;
        }
    }

    void World::syncLoad()
    {
        while (mChunksLoading)
        {
            OGRE_THREAD_SLEEP(0);
            Ogre::Root::getSingleton().getWorkQueue()->processResponses();
        }
    }

    Ogre::WorkQueue::Response* World::handleRequest(const Ogre::WorkQueue::Request *req, const Ogre::WorkQueue *srcQ)
    {
        const LoadRequestData data = Ogre::any_cast<LoadRequestData>(req->getData());

        QuadTreeNode* node = data.mNode;

        LoadResponseData* responseData = new LoadResponseData();

        Ogre::Timer timer;
        getStorage()->fillVertexBuffers(node->getNativeLodLevel(), node->getSize(), node->getCenter(), getAlign(),
                                        responseData->mPositions, responseData->mNormals, responseData->mColours);

        std::cout << "THREAD" << std::endl;

        responseData->time = timer.getMicroseconds();

        return OGRE_NEW Ogre::WorkQueue::Response(req, true, Ogre::Any(responseData));
    }

    void World::handleResponse(const Ogre::WorkQueue::Response *res, const Ogre::WorkQueue *srcQ)
    {
        static unsigned long time = 0;
        if (res->succeeded())
        {
            LoadResponseData* data = Ogre::any_cast<LoadResponseData*>(res->getData());

            const LoadRequestData requestData = Ogre::any_cast<LoadRequestData>(res->getRequest()->getData());

            requestData.mNode->load(*data);

            time += data->time;

            delete data;

            std::cout << "RESPONSE, reqs took ms" << time/1000.f << std::endl;
        }
        --mChunksLoading;
    }

    void World::queueLoad(QuadTreeNode *node)
    {
        LoadRequestData data;
        data.mNode = node;
        data.mPack = getShadersEnabled();

        const Ogre::uint16 loadRequestId = 1;
        Ogre::Root::getSingleton().getWorkQueue()->addRequest(mWorkQueueChannel, loadRequestId, Ogre::Any(data));
        ++mChunksLoading;
    }
}
