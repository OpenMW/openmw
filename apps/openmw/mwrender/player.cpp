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
      mFirstPersonView(true),
      mVanityMode(false),
      mPreviewMode(false)
    {
        Ogre::SceneNode *pitchNode = mCameraNode->createChildSceneNode();
        pitchNode->attachObject(mCamera);
    }
    
    void Player::rotateImpl(Ogre::Vector3 &rot, bool adjust, float r)
    {
        rotateCamera(rot, adjust);

        if (mVanityMode || mPreviewMode || !mFirstPersonView) {
            moveCamera(r);
        }
        updateListener();
    }

    bool Player::rotate(const Ogre::Vector3 &rot, bool adjust)
    {
        Ogre::Vector3 _rot = rot;
        rotateImpl(_rot, adjust, 400.f);

        mUpdates = 0;
        mTimeIdle = 0.f;

        if (mVanityMode) {
            toggleVanityMode();
        }

        return !mVanityMode && !mPreviewMode;
    }

    void Player::rotateCamera(Ogre::Vector3 &rot, bool adjust)
    {
        Ogre::SceneNode *pitchNode = mCamera->getParentSceneNode();
        Ogre::SceneNode *yawNode = pitchNode->getParentSceneNode();

        if (adjust) {
            float f =
                limitPitchAngle(89.5f, Ogre::Radian(rot.x).valueDegrees());

            if (f != 0.0) {
                pitchNode->pitch(Ogre::Degree(f));
            }
            yawNode->yaw(Ogre::Radian(-rot.z));
        } else {
            Ogre::Radian radx(rot.x);
            if (radx.valueDegrees() > 89.5f) {
                radx = Ogre::Degree(89.5f);
            } else if (radx.valueDegrees() < -89.5f) {
                radx = Ogre::Degree(-89.5f);
            }
            Ogre::Quaternion xr(radx, Ogre::Vector3::UNIT_X);
            Ogre::Quaternion yr(Ogre::Radian(-rot.z), Ogre::Vector3::UNIT_Y);

            pitchNode->setOrientation(xr);
            yawNode->setOrientation(yr);
        }
    }

    void Player::moveCamera(float r)
    {
/*
        Ogre::Quaternion orient =
            mCamera->getParentSceneNode()->getOrientation();

        float pitchAngle =
            (2 * Ogre::Degree(Ogre::Math::ASin(orient.x)).valueDegrees());

        orient = mCameraNode->getOrientation();
        float yawAngle =
            (2 * Ogre::Degree(Ogre::Math::ASin(orient.y)).valueDegrees());
    
        float tana = Ogre::Math::Tan(Ogre::Degree(pitchAngle));
        float tansq = tana * tana;

        float r1 = hsq * rsq / (hsq + rsq * tansq);
        float zsq = r1 * tansq;
        r1 = Ogre::Math::Sqrt(r1);

        Ogre::Vector3 pos;
        pos.y = -Ogre::Math::Sqrt(zsq);
        pos.z = r1 * Ogre::Math::Sin(Ogre::Degree(yawAngle).valueDegrees());
        pos.x = r1 * Ogre::Math::Cos(Ogre::Degree(yawAngle).valueDegrees());
*/
        Ogre::Vector3 dir = mCamera->getRealDirection();
        dir.x = -dir.x, dir.y = -dir.y, dir.z = -dir.z;

        Ogre::Ray ray(Ogre::Vector3(0, 0, 0), dir);

        mCameraNode->setPosition(ray.getPoint(r));
    }

    std::string Player::getHandle() const
    {
        return mPlayerNode->getName();
    }

    void Player::attachTo(const MWWorld::Ptr &ptr)
    {
        ptr.getRefData().setBaseNode(mPlayerNode);
    }

    float Player::limitPitchAngle(float limitAbs, float shift)
    {
        Ogre::Quaternion orient =
            mCamera->getParentSceneNode()->getOrientation();

        float pitchAngle =
            (2 * Ogre::Degree(Ogre::Math::ASin(orient.x)).valueDegrees());

        if (pitchAngle + shift < limitAbs && pitchAngle + shift > -limitAbs) {
            return shift;
        }
        if (pitchAngle > 0) {
            float f = limitAbs - pitchAngle - shift;
            return (f > 0.f) ? f : 0.f;
        } else if (pitchAngle < 0) {
            float f = -limitAbs - pitchAngle - shift;
            return (f < 0.f) ? f : 0.f;
        }
        return 0.f;
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
                toggleVanityMode();
            }
        }
        if (mFirstPersonView && !mVanityMode) {
            return;
        }
        if (mVanityMode) {
            Ogre::Vector3 rot(0.f, 0.f, 0.f);
            rot.z = Ogre::Degree(3.f * duration).valueRadians();
            rotateImpl(rot, true, 300.f);
        }
    }

    void Player::toggleViewMode()
    {
        mFirstPersonView = !mFirstPersonView;
        if (mFirstPersonView) {
            mCameraNode->setPosition(0.f, 0.f, 0.f);
        } else {
            moveCamera(400.f);
        }
    }

    void Player::toggleVanityMode()
    {
        mVanityMode = !mVanityMode;

        float r = 400.f;
        Ogre::Vector3 rot(0.f, 0.f, 0.f);
        if (mVanityMode) {
            mPitch = getPitchAngle();
            mYaw = getYawAngle();

            rot.x = Ogre::Degree(-30.f).valueRadians();
            rot.z = 0;
            r = 300.f;
        } else {
            rot.x = Ogre::Degree(mPitch).valueRadians();
            rot.z = Ogre::Degree(mYaw).valueRadians();
        }
        rotateImpl(rot, false, r);
    }

    void Player::togglePreviewMode()
    {
        /// \todo move camera
        mPreviewMode = !mPreviewMode;
    }

    float Player::getPitchAngle()
    {
        Ogre::Quaternion orient
            = mCamera->getParentSceneNode()->getOrientation();

        float angle =
            (2 * Ogre::Degree(Ogre::Math::ASin(orient.x)).valueDegrees());

        return angle;
    }

    float Player::getYawAngle()
    {
        Ogre::Quaternion orient
            = mCameraNode->getOrientation();

        float angle =
            (2 * Ogre::Degree(Ogre::Math::ASin(orient.y)).valueDegrees());
        if (orient.w < 0) {
            angle = -angle;
        }
        return -angle;
    }
}
