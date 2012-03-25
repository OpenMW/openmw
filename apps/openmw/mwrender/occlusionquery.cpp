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
    matQueryVisible->setColourWriteEnabled(false); // Uncomment this to visualize the occlusion query
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
    mBBQuerySingleObject->setDefaultDimensions(0.01, 0.01);
    mBBQuerySingleObject->createBillboard(Vector3::ZERO);
    mBBQuerySingleObject->setMaterialName("QueryVisiblePixels");
    mBBQuerySingleObject->setRenderQueueGroup(queue);
    mObjectNode->attachObject(mBBQuerySingleObject);

    mRendering->getScene()->addRenderObjectListener(this);
    mRendering->getScene()->addRenderQueueListener(this);
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
        {
            mActiveQuery = mSunTotalAreaQuery;
            mWasVisible = true;
        }
        else if (rend == mBBQueryVisible)
        {
            mActiveQuery = mSunVisibleAreaQuery;
        }
    }
    if (rend == mBBQuerySingleObject && mQuerySingleObjectRequested)
    {
        mQuerySingleObjectStarted = true;
        mQuerySingleObjectRequested = false;
        mActiveQuery = mSingleObjectQuery;
    }
    
    if (mActiveQuery != NULL)
        mActiveQuery->beginOcclusionQuery();
}

void OcclusionQuery::renderQueueEnded(uint8 queueGroupId, const String& invocation, bool& repeatThisInvocation)
{    
    if (queueGroupId == RENDER_QUEUE_SKIES_LATE && mWasVisible == false)
    {
        // for some reason our single object query returns wrong results when the sun query was never executed
        // (which can happen when we are in interiors, or when the sun is outside of the view frustum and gets culled)
        // so we force it here once everything has been rendered
        mSunTotalAreaQuery->beginOcclusionQuery();
        mSunTotalAreaQuery->endOcclusionQuery();
        mSunVisibleAreaQuery->beginOcclusionQuery();
        mSunVisibleAreaQuery->endOcclusionQuery();
    } 
}

void OcclusionQuery::update()
{
    if (!mSupported) return;

    mWasVisible = false;

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
        && !mSunVisibleAreaQuery->isStillOutstanding())
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

        mDoQuery = true;
    }
    if (!mSingleObjectQuery->isStillOutstanding() && mQuerySingleObjectStarted)
    {
        unsigned int result;

        mSingleObjectQuery->pullOcclusionQuery(&result);

        //std::cout << "Single object query result: " << result << " pixels " << std::endl;
        mTestResult = (result != 0);

        mBBQuerySingleObject->setVisible(false);

        // restore old render queues
        for (std::vector<ObjectInfo>::iterator it=mObjectsInfo.begin();
            it!=mObjectsInfo.end(); ++it)
        {
            if (!mRendering->getScene()->hasMovableObject((*it).name, (*it).typeName)) return;
            mRendering->getScene()->getMovableObject((*it).name, (*it).typeName)->setRenderQueueGroup( (*it).oldRenderqueue );
        }

        mQuerySingleObjectStarted = false;
        mQuerySingleObjectRequested = false;
    }
}

void OcclusionQuery::occlusionTest(const Ogre::Vector3& position, Ogre::SceneNode* object)
{
    assert( !occlusionTestPending()
        && "Occlusion test still pending");

    mBBQuerySingleObject->setVisible(true);

    // we don't want the object to occlude itself
    // put it in a render queue _after_ the occlusion query
    mObjectsInfo.clear();
    for (int i=0; i<object->numAttachedObjects(); ++i)
    {
        ObjectInfo info;
        MovableObject* obj = object->getAttachedObject(i);
        info.name = obj->getName();
        info.typeName = obj->getMovableType();
        info.oldRenderqueue = obj->getRenderQueueGroup();

        mObjectsInfo.push_back(info);

        object->getAttachedObject(i)->setRenderQueueGroup(RENDER_QUEUE_MAIN+5);
    }

    mObjectNode->setPosition(position);
    // scale proportional to camera distance, in order to always give the billboard the same size in screen-space
    mObjectNode->setScale( Vector3(1,1,1)*(position - mRendering->getCamera()->getRealPosition()).length() );

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
