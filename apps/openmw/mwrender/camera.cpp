#include "camera.hpp"

#include <OgreSceneNode.h>
#include <OgreCamera.h>
#include <OgreSceneManager.h>
#include <OgreTagPoint.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/refdata.hpp"

#include "npcanimation.hpp"

namespace MWRender
{
    Camera::Camera (Ogre::Camera *camera)
    : mCamera(camera),
      mCameraNode(NULL),
      mCameraPosNode(NULL),
      mAnimation(NULL),
      mFirstPersonView(true),
      mPreviewMode(false),
      mFreeLook(true),
      mNearest(30.f),
      mFurthest(800.f),
      mIsNearest(false),
      mHeight(124.f),
      mCameraDistance(192.f),
      mDistanceAdjusted(false),
      mVanityToggleQueued(false),
      mViewModeToggleQueued(false)
    {
        mVanity.enabled = false;
        mVanity.allowed = true;

        mPreviewCam.pitch = 0.f;
        mPreviewCam.yaw = 0.f;
        mPreviewCam.offset = 400.f;
        mMainCam.pitch = 0.f;
        mMainCam.yaw = 0.f;
        mMainCam.offset = 400.f;
    }

    Camera::~Camera()
    {
    }

    void Camera::reset()
    {
        togglePreviewMode(false);
        toggleVanityMode(false);
        if (!mFirstPersonView)
            toggleViewMode();
    }

    void Camera::rotateCamera(const Ogre::Vector3 &rot, bool adjust)
    {
        if (adjust) {
            setYaw(getYaw() + rot.z);
            setPitch(getPitch() + rot.x);
        } else {
            setYaw(rot.z);
            setPitch(rot.x);
        }

        Ogre::Quaternion xr(Ogre::Radian(getPitch() + Ogre::Math::HALF_PI), Ogre::Vector3::UNIT_X);
        Ogre::Quaternion orient = xr;
        if (mVanity.enabled || mPreviewMode) {
            Ogre::Quaternion zr(Ogre::Radian(getYaw()), Ogre::Vector3::UNIT_Z);
            orient = zr * xr;
        }

        if (isFirstPerson())
            mCamera->getParentNode()->setOrientation(orient);
        else
            mCameraNode->setOrientation(orient);
    }

    const std::string &Camera::getHandle() const
    {
        return mTrackingPtr.getRefData().getHandle();
    }

    Ogre::SceneNode* Camera::attachTo(const MWWorld::Ptr &ptr)
    {
        mTrackingPtr = ptr;
        Ogre::SceneNode *node = mTrackingPtr.getRefData().getBaseNode()->createChildSceneNode(Ogre::Vector3(0.0f, 0.0f, mHeight));
        node->setInheritScale(false);
        Ogre::SceneNode *posNode = node->createChildSceneNode();
        posNode->setInheritScale(false);
        if(mCameraNode)
        {
            node->setOrientation(mCameraNode->getOrientation());
            posNode->setPosition(mCameraPosNode->getPosition());
            mCameraNode->getCreator()->destroySceneNode(mCameraNode);
            mCameraNode->getCreator()->destroySceneNode(mCameraPosNode);
        }
        mCameraNode = node;
        mCameraPosNode = posNode;

        if (!isFirstPerson())
        {
            mCamera->detachFromParent();
            mCameraPosNode->attachObject(mCamera);
        }

        return mCameraPosNode;
    }

    void Camera::setPosition(const Ogre::Vector3& position)
    {
        mCameraPosNode->setPosition(position);
    }

    void Camera::setPosition(float x, float y, float z)
    {
        setPosition(Ogre::Vector3(x,y,z));
    }

    void Camera::update(float duration, bool paused)
    {
        if (mAnimation->upperBodyReady())
        {
            // Now process the view changes we queued earlier
            if (mVanityToggleQueued)
            {
                toggleVanityMode(!mVanity.enabled);
                mVanityToggleQueued = false;
            }
            if (mViewModeToggleQueued)
            {

                togglePreviewMode(false);
                toggleViewMode();
                mViewModeToggleQueued = false;
            }
        }

        if (paused)
            return;

        // only show the crosshair in game mode and in first person mode.
        MWBase::WindowManager *wm = MWBase::Environment::get().getWindowManager();
        wm->showCrosshair(!wm->isGuiMode() && (mFirstPersonView && !mVanity.enabled && !mPreviewMode));

        if(mVanity.enabled)
        {
            Ogre::Vector3 rot(0.f, 0.f, 0.f);
            rot.z = Ogre::Degree(3.f * duration).valueRadians();
            rotateCamera(rot, true);
        }
    }

    void Camera::toggleViewMode(bool force)
    {
        // Changing the view will stop all playing animations, so if we are playing
        // anything important, queue the view change for later
        if (!mAnimation->upperBodyReady() && !force)
        {
            mViewModeToggleQueued = true;
            return;
        }
        else
            mViewModeToggleQueued = false;

        mFirstPersonView = !mFirstPersonView;
        processViewChange();

        if (mFirstPersonView) {
            setPosition(0.f, 0.f, 0.f);
        } else {
            setPosition(0.f, 0.f, mCameraDistance);
        }
    }
    
    void Camera::allowVanityMode(bool allow)
    {
        if (!allow && mVanity.enabled)
            toggleVanityMode(false);
        mVanity.allowed = allow;
    }

    bool Camera::toggleVanityMode(bool enable)
    {
        // Changing the view will stop all playing animations, so if we are playing
        // anything important, queue the view change for later
        if (isFirstPerson() && !mAnimation->upperBodyReady())
        {
            mVanityToggleQueued = true;
            return false;
        }

        if(!mVanity.allowed && enable)
            return false;

        if(mVanity.enabled == enable)
            return true;
        mVanity.enabled = enable;

        processViewChange();

        float offset = mPreviewCam.offset;
        Ogre::Vector3 rot(0.f, 0.f, 0.f);
        if (mVanity.enabled) {
            rot.x = Ogre::Degree(-30.f).valueRadians();
            mMainCam.offset = mCameraPosNode->getPosition().z;
        } else {
            rot.x = getPitch();
            offset = mMainCam.offset;
        }
        rot.z = getYaw();

        setPosition(0.f, 0.f, offset);
        rotateCamera(rot, false);

        return true;
    }

    void Camera::togglePreviewMode(bool enable)
    {
        if (mFirstPersonView && !mAnimation->upperBodyReady())
            return;

        if(mPreviewMode == enable)
            return;

        mPreviewMode = enable;
        processViewChange();

        float offset = mCameraPosNode->getPosition().z;
        if (mPreviewMode) {
            mMainCam.offset = offset;
            offset = mPreviewCam.offset;
        } else {
            mPreviewCam.offset = offset;
            offset = mMainCam.offset;
        }

        setPosition(0.f, 0.f, offset);
    }

    void Camera::setSneakOffset(float offset)
    {
        if(mAnimation)
            mAnimation->addFirstPersonOffset(Ogre::Vector3(0.f, 0.f, -offset));
    }

    float Camera::getYaw()
    {
        if(mVanity.enabled || mPreviewMode)
            return mPreviewCam.yaw;
        return mMainCam.yaw;
    }

    void Camera::setYaw(float angle)
    {
        if (angle > Ogre::Math::PI) {
            angle -= Ogre::Math::TWO_PI;
        } else if (angle < -Ogre::Math::PI) {
            angle += Ogre::Math::TWO_PI;
        }
        if (mVanity.enabled || mPreviewMode) {
            mPreviewCam.yaw = angle;
        } else {
            mMainCam.yaw = angle;
        }
    }

    float Camera::getPitch()
    {
        if (mVanity.enabled || mPreviewMode) {
            return mPreviewCam.pitch;
        }
        return mMainCam.pitch;
    }

    void Camera::setPitch(float angle)
    {
        const float epsilon = 0.000001f;
        float limit = Ogre::Math::HALF_PI - epsilon;
        if(mPreviewMode)
            limit /= 2;

        if(angle > limit)
            angle = limit;
        else if(angle < -limit)
            angle = -limit;

        if (mVanity.enabled || mPreviewMode) {
            mPreviewCam.pitch = angle;
        } else {
            mMainCam.pitch = angle;
        }
    }

    float Camera::getCameraDistance() const
    {
        return mCameraPosNode->getPosition().z;
    }

    void Camera::setCameraDistance(float dist, bool adjust, bool override)
    {
        if(mFirstPersonView && !mPreviewMode && !mVanity.enabled)
            return;

        mIsNearest = false;

        Ogre::Vector3 v(0.f, 0.f, dist);
        if (adjust) {
            v += mCameraPosNode->getPosition();
        }
        if (v.z >= mFurthest) {
            v.z = mFurthest;
        } else if (!override && v.z < 10.f) {
            v.z = 10.f;
        } else if (override && v.z <= mNearest) {
            v.z = mNearest;
            mIsNearest = true;
        }
        setPosition(v);

        if (override) {
            if (mVanity.enabled || mPreviewMode) {
                mPreviewCam.offset = v.z;
            } else if (!mFirstPersonView) {
                mCameraDistance = v.z;
            }
        } else {
            mDistanceAdjusted = true;
        }
    }

    void Camera::setCameraDistance()
    {
        if (mDistanceAdjusted) {
            if (mVanity.enabled || mPreviewMode) {
                setPosition(0, 0, mPreviewCam.offset);
            } else if (!mFirstPersonView) {
                setPosition(0, 0, mCameraDistance);
            }
        }
        mDistanceAdjusted = false;
    }

    void Camera::setAnimation(NpcAnimation *anim)
    {
        // If we're switching to a new NpcAnimation, ensure the old one is
        // using a normal view mode
        if(mAnimation && mAnimation != anim)
        {
            mAnimation->setViewMode(NpcAnimation::VM_Normal);
            mAnimation->detachObjectFromBone(mCamera);
        }
        mAnimation = anim;

        processViewChange();
    }

    void Camera::processViewChange()
    {
        mAnimation->detachObjectFromBone(mCamera);
        mCamera->detachFromParent();

        if(isFirstPerson())
        {
            mAnimation->setViewMode(NpcAnimation::VM_FirstPerson);
            Ogre::TagPoint *tag = mAnimation->attachObjectToBone("Head", mCamera);
            tag->setInheritOrientation(false);
        }
        else
        {
            mAnimation->setViewMode(NpcAnimation::VM_Normal);
            mCameraPosNode->attachObject(mCamera);
        }
        rotateCamera(Ogre::Vector3(getPitch(), 0.f, getYaw()), false);
    }

    void Camera::getPosition(Ogre::Vector3 &focal, Ogre::Vector3 &camera)
    {
        mCamera->getParentSceneNode()->needUpdate(true);

        camera = mCamera->getRealPosition();
        focal = mCameraNode->_getDerivedPosition();
    }

    void Camera::togglePlayerLooking(bool enable)
    {
        mFreeLook = enable;
    }

    bool Camera::isVanityOrPreviewModeEnabled()
    {
        return mPreviewMode || mVanity.enabled;
    }

    bool Camera::isNearest()
    {
        return mIsNearest;
    }
}
