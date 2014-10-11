#include "occlusionquery.hpp"

#include <OgreRenderSystem.h>
#include <OgreRoot.h>
#include <OgreBillboardSet.h>
#include <OgreHardwareOcclusionQuery.h>
#include <OgreEntity.h>
#include <OgreSubEntity.h>
#include <OgreMeshManager.h>
#include <OgreMaterialManager.h>
#include <OgreCamera.h>
#include <OgreSceneNode.h>
#include <OgreMesh.h>

#include "renderconst.hpp"

using namespace MWRender;
using namespace Ogre;

OcclusionQuery::OcclusionQuery(OEngine::Render::OgreRenderer* renderer, SceneNode* sunNode) :
    mSunTotalAreaQuery(0), mSunVisibleAreaQuery(0), mActiveQuery(0),
    mBBQueryVisible(0), mBBQueryTotal(0), mSunNode(sunNode), mBBNodeReal(0),
    mSunVisibility(0),
    mWasVisible(false),
    mActive(false),
    mFirstFrame(true),
    mDoQuery(0),
    mRendering(renderer)
{
    try {
        RenderSystem* renderSystem = Root::getSingleton().getRenderSystem();

        mSunTotalAreaQuery = renderSystem->createHardwareOcclusionQuery();
        mSunVisibleAreaQuery = renderSystem->createHardwareOcclusionQuery();

        mSupported = (mSunTotalAreaQuery != 0) && (mSunVisibleAreaQuery != 0);
    }
    catch (Ogre::Exception&)
    {
        mSupported = false;
    }

    if (!mSupported)
    {
        std::cout << "Hardware occlusion queries not supported." << std::endl;
        return;
    }

    mBBNodeReal = mRendering->getScene()->getRootSceneNode()->createChildSceneNode();

    static Ogre::Mesh* plane = MeshManager::getSingleton().createPlane("occlusionbillboard",
        ResourceGroupManager::DEFAULT_RESOURCE_GROUP_NAME,  Ogre::Plane(Ogre::Vector3(0,0,1), 0), 1, 1, 1, 1, true, 1, 1, 1, Vector3::UNIT_Y).get();
    plane->_setBounds(Ogre::AxisAlignedBox::BOX_INFINITE);

    mBBQueryTotal = mRendering->getScene()->createEntity("occlusionbillboard");
    mBBQueryTotal->setCastShadows(false);
    mBBQueryTotal->setVisibilityFlags(RV_OcclusionQuery);
    mBBQueryTotal->setRenderQueueGroup(RQG_OcclusionQuery+1);
    mBBQueryTotal->setMaterialName("QueryTotalPixels");
    mBBNodeReal->attachObject(mBBQueryTotal);

    mBBQueryVisible = mRendering->getScene()->createEntity("occlusionbillboard");
    mBBQueryVisible->setCastShadows(false);
    mBBQueryVisible->setVisibilityFlags(RV_OcclusionQuery);
    mBBQueryVisible->setRenderQueueGroup(RQG_OcclusionQuery+1);
    mBBQueryVisible->setMaterialName("QueryVisiblePixels");
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
        if (rend == mBBQueryTotal->getSubEntity(0))
        {
            mActiveQuery = mSunTotalAreaQuery;
            mWasVisible = true;
        }
        else if (rend == mBBQueryVisible->getSubEntity(0))
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
    if (mFirstFrame)
    {
        // GLHardwareOcclusionQuery::isStillOutstanding doesn't seem to like getting called when nothing has been rendered yet
        mFirstFrame = false;
        return;
    }
    if (!mSupported) return;

    mWasVisible = false;

    // Adjust the position of the sun billboards according to camera viewing distance
    // we need to do this to make sure that _everything_ can occlude the sun
    float dist = mRendering->getCamera()->getFarClipDistance();
    if (dist==0) dist = 10000000;
    dist -= 1000; // bias
    dist /= 1000.f;
    if (mSunNode)
    {
        mBBNodeReal->setPosition(mSunNode->getPosition() * dist);
        mBBNodeReal->setOrientation(Ogre::Vector3::UNIT_Z.getRotationTo(-mBBNodeReal->getPosition().normalisedCopy()));
        mBBNodeReal->setScale(150.f*dist, 150.f*dist, 150.f*dist);
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
}
