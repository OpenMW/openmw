#include "occlusionquery.hpp"

#include <OgreRenderSystem.h>
#include <OgreRoot.h>
#include <OgreBillboardSet.h>
#include <OgreHardwareOcclusionQuery.h>
#include <OgreEntity.h>

using namespace MWRender;
using namespace Ogre;

OcclusionQuery::OcclusionQuery(OEngine::Render::OgreRenderer* renderer, SceneNode* sunNode) :
    mSunTotalAreaQuery(0), mSunVisibleAreaQuery(0), mSingleObjectQuery(0), mActiveQuery(0),
    mDoQuery(0), mSunVisibility(0), mQuerySingleObjectStarted(false),
    mQuerySingleObjectRequested(false)
{
    mRendering = renderer;
    mSunNode = sunNode;

    try {
        RenderSystem* renderSystem = Root::getSingleton().getRenderSystem();

        mSunTotalAreaQuery = renderSystem->createHardwareOcclusionQuery();
        mSunVisibleAreaQuery = renderSystem->createHardwareOcclusionQuery();
        mSingleObjectQuery = renderSystem->createHardwareOcclusionQuery();

        mSupported = (mSunTotalAreaQuery != 0) && (mSunVisibleAreaQuery != 0) && (mSingleObjectQuery != 0);
    }
    catch (Ogre::Exception e)
    {
        mSupported = false;
    }

    if (!mSupported)
    {
        std::cout << "Hardware occlusion queries not supported." << std::endl;
        return;
    }

    // This means that everything up to RENDER_QUEUE_MAIN can occlude the objects that are tested
    const int queue = RENDER_QUEUE_MAIN+1;

    MaterialPtr matBase = MaterialManager::getSingleton().getByName("BaseWhiteNoLighting");
    MaterialPtr matQueryArea = matBase->clone("QueryTotalPixels");
    matQueryArea->setDepthWriteEnabled(false);
    matQueryArea->setColourWriteEnabled(false);
    matQueryArea->setDepthCheckEnabled(false); // Not occluded by objects
    MaterialPtr matQueryVisible = matBase->clone("QueryVisiblePixels");
    matQueryVisible->setDepthWriteEnabled(false);
    matQueryVisible->setColourWriteEnabled(false);
    matQueryVisible->setDepthCheckEnabled(true); // Occluded by objects

    mBBNode = mSunNode->getParentSceneNode()->createChildSceneNode();

    mObjectNode = mRendering->getScene()->getRootSceneNode()->createChildSceneNode();

    mBBQueryTotal = mRendering->getScene()->createBillboardSet(1);
    mBBQueryTotal->setDefaultDimensions(150, 150);
    mBBQueryTotal->createBillboard(Vector3::ZERO);
    mBBQueryTotal->setMaterialName("QueryTotalPixels");
    mBBQueryTotal->setRenderQueueGroup(queue);
    mBBNode->attachObject(mBBQueryTotal);

    mBBQueryVisible = mRendering->getScene()->createBillboardSet(1);
    mBBQueryVisible->setDefaultDimensions(150, 150);
    mBBQueryVisible->createBillboard(Vector3::ZERO);
    mBBQueryVisible->setMaterialName("QueryVisiblePixels");
    mBBQueryVisible->setRenderQueueGroup(queue);
    mBBNode->attachObject(mBBQueryVisible);

    mBBQuerySingleObject = mRendering->getScene()->createBillboardSet(1);
    mBBQuerySingleObject->setDefaultDimensions(10, 10);
    mBBQuerySingleObject->createBillboard(Vector3::ZERO);
    mBBQuerySingleObject->setMaterialName("QueryVisiblePixels");
    mBBQuerySingleObject->setRenderQueueGroup(queue);
    mObjectNode->attachObject(mBBQuerySingleObject);

    mRendering->getScene()->addRenderObjectListener(this);
    mDoQuery = true;
}

OcclusionQuery::~OcclusionQuery()
{
    RenderSystem* renderSystem = Root::getSingleton().getRenderSystem();
    if (mSunTotalAreaQuery) renderSystem->destroyHardwareOcclusionQuery(mSunTotalAreaQuery);
    if (mSunVisibleAreaQuery) renderSystem->destroyHardwareOcclusionQuery(mSunVisibleAreaQuery);
    if (mSingleObjectQuery) renderSystem->destroyHardwareOcclusionQuery(mSingleObjectQuery);
}

bool OcclusionQuery::supported()
{
    return mSupported;
}

void OcclusionQuery::notifyRenderSingleObject(Renderable* rend, const Pass* pass, const AutoParamDataSource* source, 
			const LightList* pLightList, bool suppressRenderStateChanges)
{
    if (!mSupported) return;

    // The following code activates and deactivates the occlusion queries
    // so that the queries only include the rendering of their intended targets

    // Close the last occlusion query
    // Each occlusion query should only last a single rendering
    if (mActiveQuery != NULL)
    {
        mActiveQuery->endOcclusionQuery();
        mActiveQuery = NULL;
    }

    // Open a new occlusion query
    if (mDoQuery == true)
    {
        if (rend == mBBQueryTotal) 
            mActiveQuery = mSunTotalAreaQuery;
        else if (rend == mBBQueryVisible) 
            mActiveQuery = mSunVisibleAreaQuery;
        else if (rend == mBBQuerySingleObject && mQuerySingleObjectRequested)
        {
            mQuerySingleObjectStarted = true;
            mQuerySingleObjectRequested = false;
            mActiveQuery = mSingleObjectQuery;
        }

        if (mActiveQuery != NULL)
            mActiveQuery->beginOcclusionQuery();
    }
}

void OcclusionQuery::update()
{
    if (!mSupported) return;

    // Adjust the position of the sun billboards according to camera viewing distance
    // we need to do this to make sure that _everything_ can occlude the sun
    float dist = mRendering->getCamera()->getFarClipDistance();
    if (dist==0) dist = 10000000;
    dist -= 1000; // bias
    dist /= 1000.f;
    mBBNode->setPosition(mSunNode->getPosition() * dist);
    mBBNode->setScale(dist, dist, dist);

    // Stop occlusion queries until we get their information
    // (may not happen on the same frame they are requested in)
    mDoQuery = false;

    if (!mSunTotalAreaQuery->isStillOutstanding()
        && !mSunVisibleAreaQuery->isStillOutstanding()
        && !mSingleObjectQuery->isStillOutstanding())
    {
        unsigned int totalPixels;
        unsigned int visiblePixels;

        mSunTotalAreaQuery->pullOcclusionQuery(&totalPixels);
        mSunVisibleAreaQuery->pullOcclusionQuery(&visiblePixels);

        if (totalPixels == 0)
        {
            // probably outside of the view frustum
            mSunVisibility = 0;
        }
        else
        {
            mSunVisibility = float(visiblePixels) / float(totalPixels);
            if (mSunVisibility > 1) mSunVisibility = 1;
        }

        if (mQuerySingleObjectStarted)
        {
            unsigned int visiblePixels;

            mSingleObjectQuery->pullOcclusionQuery(&visiblePixels);

            mBBQuerySingleObject->setVisible(false);
            mObject->setRenderQueueGroup(mObjectOldRenderQueue);

            mQuerySingleObjectStarted = false;
            mQuerySingleObjectRequested = false;
        }

        mDoQuery = true;
    }
}

void OcclusionQuery::occlusionTest(const Ogre::Vector3& position, Ogre::Entity* entity)
{
    assert( !occlusionTestPending()
        && "Occlusion test still pending");

    mBBQuerySingleObject->setVisible(true);

    // we don't want the object to occlude itself
    mObjectOldRenderQueue = entity->getRenderQueueGroup();
    if (mObjectOldRenderQueue < RENDER_QUEUE_MAIN+2)
        entity->setRenderQueueGroup(RENDER_QUEUE_MAIN+2);

    mObjectNode->setPosition(position);

    mQuerySingleObjectRequested = true;
}

bool OcclusionQuery::occlusionTestPending()
{
    return (mQuerySingleObjectRequested || mQuerySingleObjectStarted);
}

bool OcclusionQuery::getTestResult()
{
    assert( !occlusionTestPending()
        && "Occlusion test still pending");

    return mTestResult;
}
