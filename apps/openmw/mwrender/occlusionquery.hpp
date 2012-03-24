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

        bool supported();
        ///< returns true if occlusion queries are supported on the user's hardware

        void update();
        ///< per-frame update

        float getSunVisibility() const {return mSunVisibility;};

    private:
        Ogre::HardwareOcclusionQuery* mSunTotalAreaQuery;
        Ogre::HardwareOcclusionQuery* mSunVisibleAreaQuery;
        Ogre::HardwareOcclusionQuery* mActiveQuery;

        Ogre::BillboardSet* mBBQueryVisible;
        Ogre::BillboardSet* mBBQueryTotal;

        Ogre::SceneNode* mSunNode;

        float mSunVisibility;

        bool mSupported;
        bool mDoQuery;

        OEngine::Render::OgreRenderer* mRendering;

    protected:
        virtual void notifyRenderSingleObject(Ogre::Renderable* rend, const Ogre::Pass* pass, const Ogre::AutoParamDataSource* source, 
			const Ogre::LightList* pLightList, bool suppressRenderStateChanges);
    };
}

#endif
