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
         * make sure to disable occlusion queries before updating unrelated render targets
         * @param active
         */
        void setActive (bool active) { mActive = active; }

        /**
         * per-frame update
         */
        void update(float duration);

        float getSunVisibility() const {return mSunVisibility;};

        void setSunNode(Ogre::SceneNode* node);

    private:
        Ogre::HardwareOcclusionQuery* mSunTotalAreaQuery;
        Ogre::HardwareOcclusionQuery* mSunVisibleAreaQuery;
        Ogre::HardwareOcclusionQuery* mActiveQuery;

        Ogre::BillboardSet* mBBQueryVisible;
        Ogre::BillboardSet* mBBQueryTotal;

        Ogre::SceneNode* mSunNode;
        Ogre::SceneNode* mBBNode;
        Ogre::SceneNode* mBBNodeReal;
        float mSunVisibility;

        bool mWasVisible;

        bool mActive;

        bool mSupported;
        bool mDoQuery;

        OEngine::Render::OgreRenderer* mRendering;

    protected:
        virtual void notifyRenderSingleObject(Ogre::Renderable* rend, const Ogre::Pass* pass, const Ogre::AutoParamDataSource* source, 
			const Ogre::LightList* pLightList, bool suppressRenderStateChanges);

        virtual void renderQueueEnded(Ogre::uint8 queueGroupId, const Ogre::String& invocation, bool& repeatThisInvocation);
    };
}

#endif
