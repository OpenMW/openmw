#include "player.hpp"

#include <OgreSceneNode.h>
#include <OgreCamera.h>
#include <OgreRay.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/refdata.hpp"

namespace MWRender
{
    Player::Player (Ogre::Camera *camera, Ogre::SceneNode* node)
    : mCamera(camera),
      mPlayerNode(node),
      mCameraNode(mPlayerNode->createChildSceneNode()),
      mVanityNode(mPlayerNode->createChildSceneNode()),
      mFirstPersonView(true),
      mVanityMode(false),
      mPreviewMode(false),
      mHeight(40.f)
    {
        mCameraNode->attachObject(mCamera);
        mCameraNode->setPosition(0.f, 0.f, mHeight);

        mPreviewCam.yaw = 0.f;
    }
    
    bool Player::rotate(const Ogre::Vector3 &rot, bool adjust)
    {
        rotateCamera(rot, adjust);

        mUpdates = 0;
        mTimeIdle = 0.f;

        if (mVanityMode) {
            toggleVanityMode(false);
        }

        return !mVanityMode && !mPreviewMode;
    }

    void Player::rotateCamera(const Ogre::Vector3 &rot, bool adjust)
    {
        Ogre::SceneNode *pitchNode = mCamera->getParentSceneNode();
        Ogre::SceneNode *yawNode = pitchNode->getParentSceneNode();

        if (adjust) {
            setYaw(getYaw() + rot.z);
            setPitch(getPitch() + rot.x);
        } else {
            setYaw(rot.z);
            setPitch(rot.x);
        }
        Ogre::Quaternion xr(
            Ogre::Radian(getPitch() + Ogre::Math::HALF_PI),
            Ogre::Vector3::UNIT_X
        );
        Ogre::Quaternion zr(Ogre::Radian(getYaw()), Ogre::Vector3::UNIT_Z);

        pitchNode->setOrientation(xr);
        yawNode->setOrientation(zr);

        updateListener();
    }

    std::string Player::getHandle() const
    {
        return mPlayerNode->getName();
    }

    void Player::attachTo(const MWWorld::Ptr &ptr)
    {
        ptr.getRefData().setBaseNode(mPlayerNode);
    }

    void Player::updateListener()
    {
        Ogre::Vector3 pos = mCamera->getRealPosition();
        Ogre::Vector3 dir = mCamera->getRealDirection();

        Ogre::Real xch;
        xch = pos.y, pos.y = -pos.z, pos.z = xch;
        xch = dir.y, dir.y = -dir.z, dir.z = xch;

        MWBase::Environment::get().getSoundManager()->setListenerPosDir(pos, dir);
    }

    void Player::update(float duration)
    {
        if (!mVanityMode) {
            ++mUpdates;
            mTimeIdle += duration;
            if (mTimeIdle > 30.f) {
                toggleVanityMode(true);
            }
        }
        if (mFirstPersonView && !mVanityMode) {
            return;
        }
        if (mVanityMode) {
            Ogre::Vector3 rot(0.f, 0.f, 0.f);
            rot.z = Ogre::Degree(3.f * duration).valueRadians();
            rotateCamera(rot, true);
        }
    }

    void Player::toggleViewMode()
    {
        mFirstPersonView = !mFirstPersonView;
        if (mFirstPersonView) {
            mCamera->setPosition(0.f, 0.f, 0.f);
        } else {
            mCamera->setPosition(0.f, 0.f, 400.f);
        }
    }

    void Player::toggleVanityMode(bool enable, bool force)
    {
        if (mVanityMode == enable) {
            return;
        }
        mVanityMode = enable;

        float offset = 300.f;
        Ogre::Vector3 rot(0.f, 0.f, 0.f);
        if (mVanityMode) {
            rot.x = Ogre::Degree(-30.f).valueRadians();
            mMainCam.offset = mCamera->getPosition().z;

            mPlayerNode->removeChild(mCameraNode);
            mVanityNode->addChild(mCameraNode);
        } else {
            rot.x = getPitch();
            offset = mMainCam.offset;

            mVanityNode->removeChild(mCameraNode);
            mPlayerNode->addChild(mCameraNode);
        }
        rot.z = getYaw();
        mCamera->setPosition(0.f, 0.f, offset);
        rotateCamera(rot, false);
    }

    void Player::togglePreviewMode(bool enable)
    {
        /// \todo move camera
        if (mPreviewMode == enable) {
            return;
        }
        mPreviewMode = enable;
    }

    float Player::getYaw()
    {
        if (mVanityMode || mPreviewMode) {
            return mPreviewCam.yaw;
        }
        return mMainCam.yaw;
    }

    void Player::setYaw(float angle)
    {
        if (angle > Ogre::Math::PI) {
            angle -= Ogre::Math::TWO_PI;
        } else if (angle < -Ogre::Math::PI) {
            angle += Ogre::Math::TWO_PI;
        }
        if (mVanityMode || mPreviewMode) {
            mPreviewCam.yaw = angle;
        } else {
            mMainCam.yaw = angle;
        }
    }

    float Player::getPitch()
    {
        if (mVanityMode || mPreviewMode) {
            return mPreviewCam.pitch;
        }
        return mMainCam.pitch;
    }

    void Player::setPitch(float angle)
    {   
        if (angle > Ogre::Math::HALF_PI) {
            angle = Ogre::Math::HALF_PI - 0.01;
        } else if (angle < -Ogre::Math::HALF_PI) {
            angle = -Ogre::Math::HALF_PI + 0.01;
        }
        if (mVanityMode || mPreviewMode) {
            mPreviewCam.pitch = angle;
        } else {
            mMainCam.pitch = angle;
        }
    }
}
