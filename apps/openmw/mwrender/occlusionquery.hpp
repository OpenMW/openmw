#ifndef _GAME_OCCLUSION_QUERY_H
#define _GAME_OCCLUSION_QUERY_H

#include <OgreHardwareOcclusionQuery.h>
#include <OgreRenderObjectListener.h>

#include <openengine/ogre/renderer.hpp>

namespace MWRender
{
    ///
    /// \brief Implements hardware occlusion queries on the GPU
    ///
    class OcclusionQuery : public Ogre::RenderObjectListener
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
        void update();

        /**
         * request occlusion test for a billboard at the given position, omitting an entity
         * @param position of the billboard in ogre coordinates
         * @param entity to exclude from the occluders
         */
        void occlusionTest(const Ogre::Vector3& position, Ogre::Entity* entity);

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
        Ogre::HardwareOcclusionQuery* mActiveQuery;

        Ogre::BillboardSet* mBBQueryVisible;
        Ogre::BillboardSet* mBBQueryTotal;

        Ogre::SceneNode* mSunNode;

        Ogre::SceneNode* mBBNode;

        float mSunVisibility;

        bool mTestResult;

        bool mSupported;
        bool mDoQuery;

        OEngine::Render::OgreRenderer* mRendering;

    protected:
        virtual void notifyRenderSingleObject(Ogre::Renderable* rend, const Ogre::Pass* pass, const Ogre::AutoParamDataSource* source, 
			const Ogre::LightList* pLightList, bool suppressRenderStateChanges);
    };
}

#endif
