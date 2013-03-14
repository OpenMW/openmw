#include "occlusionquery.hpp"
#include "renderconst.hpp"

#include <OgreRenderSystem.h>
#include <OgreRoot.h>
#include <OgreBillboardSet.h>
#include <OgreHardwareOcclusionQuery.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h>
#include <OgreMaterialManager.h>

using namespace MWRender;
using namespace Ogre;

OcclusionQuery::OcclusionQuery(OEngine::Render::OgreRenderer* renderer, SceneNode* sunNode) :
    mSunTotalAreaQuery(0), mSunVisibleAreaQuery(0), mActiveQuery(0),
    mDoQuery(0), mSunVisibility(0),
    mWasVisible(false),
    mBBNode(0), mActive(false)
{
    mRendering = renderer;
    mSunNode = sunNode;

    try {
        RenderSystem* renderSystem = Root::getSingleton().getRenderSystem();

        mSunTotalAreaQuery = renderSystem->createHardwareOcclusionQuery();
        mSunVisibleAreaQuery = renderSystem->createHardwareOcclusionQuery();

        mSupported = (mSunTotalAreaQuery != 0) && (mSunVisibleAreaQuery != 0);
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

    MaterialPtr matBase = MaterialManager::getSingleton().getByName("BaseWhiteNoLighting");
    MaterialPtr matQueryArea = matBase->clone("QueryTotalPixels");
    matQueryArea->setDepthWriteEnabled(false);
    matQueryArea->setColourWriteEnabled(false);
    matQueryArea->setDepthCheckEnabled(false); // Not occluded by objects
    MaterialPtr matQueryVisible = matBase->clone("QueryVisiblePixels");
    matQueryVisible->setDepthWriteEnabled(false);
    matQueryVisible->setColourWriteEnabled(false); // Uncomment this to visualize the occlusion query
    matQueryVisible->setDepthCheckEnabled(true); // Occluded by objects
    matQueryVisible->setCullingMode(CULL_NONE);
    matQueryVisible->setManualCullingMode(MANUAL_CULL_NONE);

    if (sunNode)
        mBBNode = mSunNode->getParentSceneNode()->createChildSceneNode();

    mBBNodeReal = mRendering->getScene()->getRootSceneNode()->createChildSceneNode();

    mBBQueryTotal = mRendering->getScene()->createBillboardSet(1);
    mBBQueryTotal->setCastShadows(false);
    mBBQueryTotal->setDefaultDimensions(150, 150);
    mBBQueryTotal->createBillboard(Vector3::ZERO);
    mBBQueryTotal->setMaterialName("QueryTotalPixels");
    mBBQueryTotal->setRenderQueueGroup(RQG_OcclusionQuery+1);
    mBBQueryTotal->setVisibilityFlags(RV_OcclusionQuery);
    mBBNodeReal->attachObject(mBBQueryTotal);

    mBBQueryVisible = mRendering->getScene()->createBillboardSet(1);
    mBBQueryVisible->setCastShadows(false);
    mBBQueryVisible->setDefaultDimensions(150, 150);
    mBBQueryVisible->createBillboard(Vector3::ZERO);
    mBBQueryVisible->setMaterialName("QueryVisiblePixels");
    mBBQueryVisible->setRenderQueueGroup(RQG_OcclusionQuery+1);
    mBBQueryVisible->setVisibilityFlags(RV_OcclusionQuery);
    mBBNodeReal->attachObject(mBBQueryVisible);

    mRendering->getScene()->addRenderObjectListener(this);
    mRendering->getScene()->addRenderQueueListener(this);
    mDoQuery = true;
}

OcclusionQuery::~OcclusionQuery()
{
    mRendering->getScene()->removeRenderObjectListener (this);
    mRendering->getScene()->removeRenderQueueListener(this);

    RenderSystem* renderSystem = Root::getSingleton().getRenderSystem();
    if (mSunTotalAreaQuery)
        renderSystem->destroyHardwareOcclusionQuery(mSunTotalAreaQuery);
    if (mSunVisibleAreaQuery)
        renderSystem->destroyHardwareOcclusionQuery(mSunVisibleAreaQuery);
}

bool OcclusionQuery::supported()
{
    return mSupported;
}

void OcclusionQuery::notifyRenderSingleObject(Renderable* rend, const Pass* pass, const AutoParamDataSource* source,
			const LightList* pLightList, bool suppressRenderStateChanges)
{
    if (!mActive) return;

    // The following code activates and deactivates the occlusion queries
    // so that the queries only include the rendering of the intended meshes

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

    if (mActiveQuery != NULL)
        mActiveQuery->beginOcclusionQuery();
}

void OcclusionQuery::renderQueueEnded(uint8 queueGroupId, const String& invocation, bool& repeatThisInvocation)
{
    if (!mActive) return;

    if (mActiveQuery != NULL)
    {
        mActiveQuery->endOcclusionQuery();
        mActiveQuery = NULL;
    }
    /**
     * for every beginOcclusionQuery(), we want a respective pullOcclusionQuery() and vice versa
     * this also means that results can be wrong at other places if we pull, but beginOcclusionQuery() was never called
     * this can happen for example if the object that is tested is outside of the view frustum
     * to prevent this, check if the queries have been performed after everything has been rendered and if not, start them manually
     */
    if (queueGroupId == RQG_SkiesLate)
    {
        if (mWasVisible == false && mDoQuery)
        {
            mSunTotalAreaQuery->beginOcclusionQuery();
            mSunTotalAreaQuery->endOcclusionQuery();
            mSunVisibleAreaQuery->beginOcclusionQuery();
            mSunVisibleAreaQuery->endOcclusionQuery();
        }
    }
}

void OcclusionQuery::update(float duration)
{
    if (!mSupported) return;

    mWasVisible = false;

    // Adjust the position of the sun billboards according to camera viewing distance
    // we need to do this to make sure that _everything_ can occlude the sun
    float dist = mRendering->getCamera()->getFarClipDistance();
    if (dist==0) dist = 10000000;
    dist -= 1000; // bias
    dist /= 1000.f;
    if (mBBNode)
    {
        mBBNode->setPosition(mSunNode->getPosition() * dist);
        mBBNode->setScale(dist, dist, dist);
        mBBNodeReal->setPosition(mBBNode->_getDerivedPosition());
        mBBNodeReal->setScale(mBBNode->getScale());
    }

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
}

void OcclusionQuery::setSunNode(Ogre::SceneNode* node)
{
    mSunNode = node;
    if (!mBBNode)
        mBBNode = node->getParentSceneNode()->createChildSceneNode();
}
