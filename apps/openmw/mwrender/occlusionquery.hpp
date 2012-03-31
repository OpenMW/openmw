#ifndef _GAME_OCCLUSION_QUERY_H
#define _GAME_OCCLUSION_QUERY_H

#include <OgreRenderObjectListener.h>
#include <OgreRenderQueueListener.h>

namespace Ogre
{
    class HardwareOcclusionQuery;
    class Entity;
    class SceneNode;
}

#include <openengine/ogre/renderer.hpp>

namespace MWRender
{
    ///
    /// \brief Implements hardware occlusion queries on the GPU
    ///
    class OcclusionQuery : public Ogre::RenderObjectListener, public Ogre::RenderQueueListener
    {
    public:
        OcclusionQuery(OEngine::Render::OgreRenderer*, Ogre::SceneNode* sunNode);
        ~OcclusionQuery();

        /**
         * @return true if occlusion queries are supported on the user's hardware
         */
        bool supported();

        /**
         * per-frame update
         */
        void update(float duration);

        /**
         * request occlusion test for a billboard at the given position, omitting an entity
         * @param position of the billboard in ogre coordinates
         * @param object to exclude from the occluders
         */
        void occlusionTest(const Ogre::Vector3& position, Ogre::SceneNode* object);

        /**
         * @return true if a request is still outstanding
         */
        bool occlusionTestPending();

        /**
         * @return true if the object tested in the last request was occluded
         */
        bool getTestResult();

        float getSunVisibility() const {return mSunVisibility;};

    private:
        Ogre::HardwareOcclusionQuery* mSunTotalAreaQuery;
        Ogre::HardwareOcclusionQuery* mSunVisibleAreaQuery;
        Ogre::HardwareOcclusionQuery* mSingleObjectQuery;
        Ogre::HardwareOcclusionQuery* mActiveQuery;

        Ogre::BillboardSet* mBBQueryVisible;
        Ogre::BillboardSet* mBBQueryTotal;
        Ogre::BillboardSet* mBBQuerySingleObject;

        Ogre::SceneNode* mSunNode;
        Ogre::SceneNode* mBBNode;
        Ogre::SceneNode* mBBNodeReal;
        float mSunVisibility;

        Ogre::SceneNode* mObjectNode;

        bool mWasVisible;
        bool mObjectWasVisible;

        bool mTestResult;

        bool mSupported;
        bool mDoQuery;
        bool mDoQuery2;

        bool mQuerySingleObjectRequested;
        bool mQuerySingleObjectStarted;

        OEngine::Render::OgreRenderer* mRendering;

    protected:
        virtual void notifyRenderSingleObject(Ogre::Renderable* rend, const Ogre::Pass* pass, const Ogre::AutoParamDataSource* source, 
			const Ogre::LightList* pLightList, bool suppressRenderStateChanges);

        virtual void renderQueueEnded(Ogre::uint8 queueGroupId, const Ogre::String& invocation, bool& repeatThisInvocation);
    };
}

#endif
