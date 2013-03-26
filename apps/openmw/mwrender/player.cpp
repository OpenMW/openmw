#include "player.hpp"

#include <OgreSceneNode.h>
#include <OgreCamera.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
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
      mFirstPersonView(true),
      mPreviewMode(false),
      mFreeLook(true),
      mHeight(128.f),
      mCameraDistance(300.f),
      mDistanceAdjusted(false),
      mAnimation(NULL)
    {
        mVanity.enabled = false;
        mVanity.allowed = true;
        mVanity.forced = false;

        mCameraNode->attachObject(mCamera);
        mCameraNode->setPosition(0.f, 0.f, mHeight);

        mPreviewCam.yaw = 0.f;
        mPreviewCam.offset = 400.f;
    }

    Player::~Player()
    {
        delete mAnimation;
    }
    
    bool Player::rotate(const Ogre::Vector3 &rot, bool adjust)
    {
        if (mVanity.enabled) {
            toggleVanityMode(false);
        }

        Ogre::Vector3 trueRot = rot;

        /// \note rotate player on forced vanity
        if (mVanity.forced) {
            if (mFreeLook) {
                float diff = (adjust) ? rot.z : mMainCam.yaw - rot.z;

                mVanity.enabled = false;
                rotateCamera(rot, adjust);
                mVanity.enabled = true;

                compensateYaw(diff);
            }
            trueRot.z = 0.f;
        }

        if (mFreeLook || mVanity.enabled || mPreviewMode) {
            rotateCamera(trueRot, adjust);
        }

        /// \note if vanity mode is forced by TVM then rotate player
        return (!mVanity.enabled && !mPreviewMode) || mVanity.forced;
    }

    void Player::rotateCamera(const Ogre::Vector3 &rot, bool adjust)
    {
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
        Ogre::Quaternion zr(
            Ogre::Radian(getYaw()),
            Ogre::Vector3::NEGATIVE_UNIT_Z
        );
        if (!mVanity.enabled && !mPreviewMode) {
            mPlayerNode->setOrientation(zr);
            mCameraNode->setOrientation(xr);
        } else {
            mCameraNode->setOrientation(zr * xr);
        }
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
        Ogre::Vector3 up  = mCamera->getRealUp();

        MWBase::Environment::get().getSoundManager()->setListenerPosDir(pos, dir, up);
    }

    void Player::update(float duration)
    {
        updateListener();

        // only show the crosshair in game mode and in first person mode.
        MWBase::Environment::get().getWindowManager ()->showCrosshair
                (!MWBase::Environment::get().getWindowManager ()->isGuiMode () && (mFirstPersonView && !mVanity.enabled && !mPreviewMode));

        /// \fixme We shouldn't hide the whole model, just certain components of the character (head, chest, feet, etc)
        mPlayerNode->setVisible(mVanity.enabled || mPreviewMode || !mFirstPersonView);
        if (mFirstPersonView && !mVanity.enabled) {
            return;
        }
        if (mVanity.enabled) {
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
            setLowHeight(false);
        } else {
            mCamera->setPosition(0.f, 0.f, mCameraDistance);
            setLowHeight(true);
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

        float offset = mPreviewCam.offset;
        Ogre::Vector3 rot(0.f, 0.f, 0.f);
        if (mVanity.enabled) {
            rot.x = Ogre::Degree(-30.f).valueRadians();
            mMainCam.offset = mCamera->getPosition().z;

            setLowHeight(true);
        } else {
            rot.x = getPitch();
            offset = mMainCam.offset;

            setLowHeight(!mFirstPersonView);
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
            mMainCam.offset = offset;
            offset = mPreviewCam.offset;

            setLowHeight(true);
        } else {
            mPreviewCam.offset = offset;
            offset = mMainCam.offset;

            setLowHeight(!mFirstPersonView);
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

    void Player::setCameraDistance(float dist, bool adjust, bool override)
    {
        if (mFirstPersonView && !mPreviewMode && !mVanity.enabled) {
            return;
        }
        Ogre::Vector3 v(0.f, 0.f, dist);
        if (adjust) {
            v += mCamera->getPosition();
        }
        if (v.z > 800.f) {
            v.z = 800.f;
        } else if (v.z < 10.f) {
            v.z = 10.f;
        } else if (override && v.z < 50.f) {
            v.z = 50.f;
        }
        mCamera->setPosition(v);

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

    void Player::setCameraDistance()
    {
        if (mDistanceAdjusted) {
            if (mVanity.enabled || mPreviewMode) {
                mCamera->setPosition(0, 0, mPreviewCam.offset);
            } else if (!mFirstPersonView) {
                mCamera->setPosition(0, 0, mCameraDistance);
            }
        }
        mDistanceAdjusted = false;
    }

    void Player::setAnimation(NpcAnimation *anim)
    {
        delete mAnimation;
        mAnimation = anim;

        mPlayerNode->setVisible(mVanity.enabled || mPreviewMode || !mFirstPersonView);
    }

    void Player::setHeight(float height)
    {
        mHeight = height;
        mCameraNode->setPosition(0.f, 0.f, mHeight);
    }

    float Player::getHeight()
    {
        return mHeight * mPlayerNode->getScale().z;
    }

    bool Player::getPosition(Ogre::Vector3 &player, Ogre::Vector3 &camera)
    {
        mCamera->getParentSceneNode ()->needUpdate(true);
        camera = mCamera->getRealPosition();
        player = mPlayerNode->getPosition();

        return mFirstPersonView && !mVanity.enabled && !mPreviewMode;
    }

    Ogre::Vector3 Player::getPosition()
    {
        return mPlayerNode->getPosition();
    }

    void Player::getSightAngles(float &pitch, float &yaw)
    {
        pitch = mMainCam.pitch;
        yaw = mMainCam.yaw;
    }

    void Player::compensateYaw(float diff)
    {
        mPreviewCam.yaw -= diff;
        Ogre::Quaternion zr(
            Ogre::Radian(mPreviewCam.yaw),
            Ogre::Vector3::NEGATIVE_UNIT_Z
        );
        Ogre::Quaternion xr(
            Ogre::Radian(mPreviewCam.pitch),
            Ogre::Vector3::UNIT_X);
        mCameraNode->setOrientation(zr * xr);
    }

    void Player::togglePlayerLooking(bool enable)
    {
        mFreeLook = enable;
    }

    void Player::setLowHeight(bool low)
    {
        if (low) {
            mCameraNode->setPosition(0.f, 0.f, mHeight * 0.85);
        } else {
            mCameraNode->setPosition(0.f, 0.f, mHeight);
        }
    }

    bool Player::isVanityOrPreviewModeEnabled()
    {
        return mPreviewMode || mVanity.enabled;
    }
}
