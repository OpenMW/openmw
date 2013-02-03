#ifndef MWRENDER_REFRACTION_H
#define MWRENDER_REFRACTION_H

#include <OgrePlane.h>
#include <OgreRenderTargetListener.h>

namespace Ogre
{
    class Camera;
    class RenderTarget;
}

namespace MWRender
{

    class Refraction : public Ogre::RenderTargetListener
    {

    public:
        Refraction(Ogre::Camera* parentCamera);
        ~Refraction();

        void setWaterPlane (Ogre::Plane plane);
        void preRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);
        void postRenderTargetUpdate(const Ogre::RenderTargetEvent& evt);

    private:
        Ogre::Camera* mParentCamera;
        Ogre::Camera* mCamera;
        Ogre::RenderTarget* mRenderTarget;
        Ogre::Plane mNearClipPlane;
    };

}

#endif
