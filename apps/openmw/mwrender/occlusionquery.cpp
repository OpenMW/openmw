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
    mSunTotalAreaQuery(0), mSunVisibleAreaQuery(0), mSingleObjectQuery(0), mActiveQuery(0),
    mDoQuery(0), mSunVisibility(0), mQuerySingleObjectStarted(false), mTestResult(false),
    mQuerySingleObjectRequested(false), mWasVisible(false), mObjectWasVisible(false), mDoQuery2(false),
    mBBNode(0)
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

    mObjectNode = mRendering->getScene()->getRootSceneNode()->createChildSceneNode();
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

    mBBQuerySingleObject = mRendering->getScene()->createBillboardSet(1);
    /// \todo ideally this should occupy exactly 1 pixel on the screen
    mBBQuerySingleObject->setCastShadows(false);
    mBBQuerySingleObject->setDefaultDimensions(0.003, 0.003);
    mBBQuerySingleObject->createBillboard(Vector3::ZERO);
    mBBQuerySingleObject->setMaterialName("QueryVisiblePixels");
    mBBQuerySingleObject->setRenderQueueGroup(RQG_OcclusionQuery);
    mBBQuerySingleObject->setVisibilityFlags(RV_OcclusionQuery);
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
    if (mDoQuery == true && rend == mBBQuerySingleObject)
    {
        mQuerySingleObjectStarted = true;
        mQuerySingleObjectRequested = false;
        mActiveQuery = mSingleObjectQuery;
        mObjectWasVisible = true;
    }

    if (mActiveQuery != NULL)
        mActiveQuery->beginOcclusionQuery();
}

void OcclusionQuery::renderQueueEnded(uint8 queueGroupId, const String& invocation, bool& repeatThisInvocation)
{
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
        if (mObjectWasVisible == false && mDoQuery)
        {
            mSingleObjectQuery->beginOcclusionQuery();
            mSingleObjectQuery->endOcclusionQuery();
            mQuerySingleObjectStarted = true;
            mQuerySingleObjectRequested = false;
        }
    }
}

void OcclusionQuery::update(float duration)
{
    if (!mSupported) return;

    mWasVisible = false;
    mObjectWasVisible = false;

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

        unsigned int result;

        mSingleObjectQuery->pullOcclusionQuery(&result);

        mTestResult = (result != 0);

        mQuerySingleObjectStarted = false;
        mQuerySingleObjectRequested = false;

        mDoQuery = true;
    }
}

void OcclusionQuery::occlusionTest(const Ogre::Vector3& position, Ogre::SceneNode* object)
{
    assert( !occlusionTestPending()
        && "Occlusion test still pending");

    mBBQuerySingleObject->setVisible(true);

    mObjectNode->setPosition(position);
    // scale proportional to camera distance, in order to always give the billboard the same size in screen-space
    mObjectNode->setScale( Vector3(1,1,1)*(position - mRendering->getCamera()->getRealPosition()).length() );

    mQuerySingleObjectRequested = true;
}

bool OcclusionQuery::occlusionTestPending()
{
    return (mQuerySingleObjectRequested || mQuerySingleObjectStarted);
}

void OcclusionQuery::setSunNode(Ogre::SceneNode* node)
{
    mSunNode = node;
    if (!mBBNode)
        mBBNode = node->getParentSceneNode()->createChildSceneNode();
}

bool OcclusionQuery::getTestResult()
{
    assert( !occlusionTestPending()
        && "Occlusion test still pending");

    return mTestResult;
}

bool OcclusionQuery::isPotentialOccluder(Ogre::SceneNode* node)
{
    bool result = false;
    for (unsigned int i=0; i < node->numAttachedObjects(); ++i)
    {
        MovableObject* ob = node->getAttachedObject(i);
        std::string type = ob->getMovableType();
        if (type == "Entity")
        {
            Entity* ent = static_cast<Entity*>(ob);
            for (unsigned int j=0; j < ent->getNumSubEntities(); ++j)
            {
                // if any sub entity has a material with depth write off,
                // consider the object as not an occluder
                MaterialPtr mat = ent->getSubEntity(j)->getMaterial();

                Material::TechniqueIterator techIt = mat->getTechniqueIterator();
                while (techIt.hasMoreElements())
                {
                    Technique* tech = techIt.getNext();
                    Technique::PassIterator passIt = tech->getPassIterator();
                    while (passIt.hasMoreElements())
                    {
                        Pass* pass = passIt.getNext();

                        if (pass->getDepthWriteEnabled() == false)
                            return false;
                        else
                            result = true;
                    }
                }
            }
        }
    }
    return result;
}
