#ifndef MWRENDER_REFRACTION_H
#define MWRENDER_REFRACTION_H

#include <OgrePlane.h>
#include <OgreRenderTargetListener.h>
#include <OgreRenderQueueListener.h>

namespace Ogre
{
    class Camera;
    class RenderTarget;
}

namespace MWRender
{

    class Refraction : public Ogre::RenderTargetListener, public Ogre::RenderQueueListener
    {

    public:
        Refraction(Ogre::Camera* parentCamera);
        ~Refraction();

        void setHeight (float height);
        void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
        void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
        void setUnderwater(bool underwater) {mIsUnderwater = underwater;}
        void setActive (bool active);

        void renderQueueStarted (Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &skipThisInvocation);
        void renderQueueEnded (Ogre::uint8 queueGroupId, const Ogre::String &invocation, bool &repeatThisInvocation);

    private:
        Ogre::Camera* mParentCamera;
        Ogre::Camera* mCamera;
        Ogre::RenderTarget* mRenderTarget;
        Ogre::Plane mNearClipPlane;
        Ogre::Plane mNearClipPlaneUnderwater;
        bool mRenderActive;
        bool mIsUnderwater;
    };

}

#endif
