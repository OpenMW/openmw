#include "player.hpp"

#include <OgreSceneNode.h>
#include <OgreCamera.h>
#include <OgreRay.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/refdata.hpp"

#include "npcanimation.hpp"

namespace MWRender
{
    Player::Player (Ogre::Camera *camera, Ogre::SceneNode* node)
    : mCamera(camera),
      mPlayerNode(node),
      mCameraNode(mPlayerNode->createChildSceneNode()),
      mVanityNode(mPlayerNode->createChildSceneNode()),
      mFirstPersonView(true),
      mPreviewMode(false),
      mHeight(128.f),
      mCameraDistance(400.f)
    {
        mVanity.enabled = false;
        mVanity.allowed = true;
        mVanity.forced = false;

        mCameraNode->attachObject(mCamera);
        mCameraNode->setPosition(0.f, 0.f, mHeight);

        mPlayerNode->setVisible(false);

        mPreviewCam.yaw = 0.f;
        mPreviewCam.offset = 600.f;
    }
    
    bool Player::rotate(const Ogre::Vector3 &rot, bool adjust)
    {
        mUpdates = 0;
        mTimeIdle = 0.f;

        if (mVanity.enabled) {
            toggleVanityMode(false);
        }

        Ogre::Vector3 trueRot = rot;

        /// \note rotate player on forced vanity
        if (mVanity.forced) {
            moveCameraNode(mPlayerNode);
            mVanity.enabled = false;

            rotateCamera(rot, adjust);

            moveCameraNode(mVanityNode);
            mVanity.enabled = true;

            trueRot.z = 0.f;
        }

        rotateCamera(trueRot, adjust);

        /// \note if vanity mode is forced by TVM then rotate player
        return (!mVanity.enabled && !mPreviewMode) || mVanity.forced;
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
        Ogre::Vector3 pos = mPlayerNode->getPosition();
        if (!mVanity.enabled) {
            ++mUpdates;
            mTimeIdle += duration;
            if (mTimeIdle > 30.f) {
                toggleVanityMode(true);
            }
        }
        if (mAnimation) {
            mAnimation->runAnimation(duration);
        }
        if (mFirstPersonView && !mVanity.enabled) {
            return;
        }
        if (mVanity.enabled) {
            Ogre::Vector3 rot(0.f, 0.f, 0.f);
            rot.z = Ogre::Degree(-3.f * duration).valueRadians();
            rotateCamera(rot, true);
        }
    }

    void Player::toggleViewMode()
    {
        mFirstPersonView = !mFirstPersonView;
        if (mFirstPersonView) {
            mCamera->setPosition(0.f, 0.f, 0.f);
            mCameraNode->setPosition(0.f, 0.f, 128.f);
            mPlayerNode->setVisible(false);
        } else {
            mCamera->setPosition(0.f, 0.f, mCameraDistance);
            mCameraNode->setPosition(0.f, 0.f, 104.f);
            mPlayerNode->setVisible(true);
        }
    }
    
    void Player::allowVanityMode(bool allow)
    {
        if (!allow && mVanity.enabled && !mVanity.forced) {
            toggleVanityMode(false);
        }
        mVanity.allowed = allow;
    }

    bool Player::toggleVanityMode(bool enable, bool force)
    {
        if ((mVanity.forced && !force) ||
            (!mVanity.allowed && (force || enable)))
        { 
            return false;
        } else if (mVanity.enabled == enable) {
            return true;
        }
        mVanity.enabled = enable;
        mVanity.forced = force && enable;

        float offset = 300.f;
        Ogre::Vector3 rot(0.f, 0.f, 0.f);
        if (mVanity.enabled) {
            mPlayerNode->setVisible(true);
            rot.x = Ogre::Degree(-30.f).valueRadians();
            mMainCam.offset = mCamera->getPosition().z;

            moveCameraNode(mVanityNode);
        } else {
            rot.x = getPitch();
            offset = mMainCam.offset;

            moveCameraNode(mPlayerNode);
        }
        if (offset == 0.f) {
            mPlayerNode->setVisible(false);
        }
        rot.z = getYaw();
        mCamera->setPosition(0.f, 0.f, offset);
        rotateCamera(rot, false);

        return true;
    }

    void Player::togglePreviewMode(bool enable)
    {
        if (mPreviewMode == enable) {
            return;
        }
        mPreviewMode = enable;
        float offset = mCamera->getPosition().z;
        if (mPreviewMode) {
            mPlayerNode->setVisible(true);
            mMainCam.offset = offset;
            offset = mPreviewCam.offset;

            moveCameraNode(mVanityNode);
        } else {
            mPreviewCam.offset = offset;
            offset = mMainCam.offset;

            moveCameraNode(mPlayerNode);
        }
        if (offset == 0.f) {
            mPlayerNode->setVisible(false);
        }
        mCamera->setPosition(0.f, 0.f, offset);
        rotateCamera(Ogre::Vector3(getPitch(), 0.f, getYaw()), false);
    }

    float Player::getYaw()
    {
        if (mVanity.enabled || mPreviewMode) {
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
        if (mVanity.enabled || mPreviewMode) {
            mPreviewCam.yaw = angle;
        } else {
            mMainCam.yaw = angle;
        }
    }

    float Player::getPitch()
    {
        if (mVanity.enabled || mPreviewMode) {
            return mPreviewCam.pitch;
        }
        return mMainCam.pitch;
    }

    void Player::setPitch(float angle)
    {
        float limit = Ogre::Math::HALF_PI;
        if (mVanity.forced || mPreviewMode) {
             limit /= 2;
        }
        if (angle > limit) {
            angle = limit - 0.01;
        } else if (angle < -limit) {
            angle = -limit + 0.01;
        }
        if (mVanity.enabled || mPreviewMode) {
            mPreviewCam.pitch = angle;
        } else {
            mMainCam.pitch = angle;
        }
    }

    void Player::moveCameraNode(Ogre::SceneNode *node)
    {
        mCameraNode->getParentSceneNode()->removeChild(mCameraNode);
        node->addChild(mCameraNode);
    }

    void Player::setCameraDistance(float dist, bool adjust)
    {
        /// \note non-Morrowind feature: allow to change camera distance
        /// int 3d-person mode
        /// \todo review and simplify condition if possible
        if (mPreviewMode ||
            mVanity.forced ||
            (!mVanity.enabled && !mFirstPersonView))
        {
            Ogre::Vector3 v(0.f, 0.f, dist);
            if (adjust) {
                v += mCamera->getPosition();
            }
            if (v.z > 800.f) {
                v.z = 800.f;
            } else if (v.z < 100.f) {
                v.z = 100.f;
            }
            mCamera->setPosition(v);

            if (!mVanity.enabled && !mFirstPersonView) {
                mCameraDistance = v.z;
            }
        }
    }

    void Player::setHeight(float height)
    {
        mHeight = height;
        mCameraNode->setPosition(0.f, 0.f, mHeight);
    }

    float Player::getHeight()
    {
        return mHeight;
    }
}
