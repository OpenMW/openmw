#include "player.hpp"

#include <OgreSceneNode.h>
#include <OgreCamera.h>

#include "../mwbase/environment.hpp"
#include "../mwbase/soundmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/refdata.hpp"

namespace MWRender
{
    Player::Player (Ogre::Camera *camera, Ogre::SceneNode* node)
    : mCamera (camera),
      mNode (node),
      mFirstPersonView(true),
      mVanityModeEnabled(false)
    {}

    bool Player::setRotation(const Ogre::Vector3 &rot)
    {
        Ogre::SceneNode *sceneNode = mNode;
        Ogre::Node* yawNode = sceneNode->getChildIterator().getNext();
        Ogre::Node* pitchNode = yawNode->getChildIterator().getNext();

        // we are only interested in X and Y rotation

        // Rotate around X axis
        Ogre::Radian radx(rot.x);
        if (radx.valueDegrees() > 89.5f) {
            radx = Ogre::Degree(89.5f);
        } else if (radx.valueDegrees() < -89.5f) {
            radx = Ogre::Degree(-89.5f);
        }
        Ogre::Quaternion xr(radx, Ogre::Vector3::UNIT_X);

        // Rotate around Y axis
        Ogre::Quaternion yr(Ogre::Radian(-rot.z), Ogre::Vector3::UNIT_Y);

        pitchNode->setOrientation(xr);
        yawNode->setOrientation(yr);

        updateListener();

        return !mVanityModeEnabled;
    }

    std::string Player::getHandle() const
    {
        return mNode->getName();
    }

    void Player::attachTo(const MWWorld::Ptr &ptr)
    {
        ptr.getRefData().setBaseNode(mNode);
    }

    bool Player::adjustRotation(const Ogre::Vector3 &rot)
    {
        Ogre::SceneNode *pitchNode = mCamera->getParentSceneNode();
        Ogre::SceneNode *yawNode = pitchNode->getParentSceneNode();

        float f = controlFlip(Ogre::Radian(rot.x).valueDegrees());
        if (f != 0.0) {
            pitchNode->pitch(Ogre::Degree(f));
        }
        yawNode->yaw(Ogre::Radian(-rot.z));

        updateListener();

        return !mVanityModeEnabled;
    }

    float Player::controlFlip(float shift)
    {
        Ogre::SceneNode *pitchNode = mCamera->getParentSceneNode();
        Ogre::Quaternion orient = pitchNode->getOrientation();

        float pitchAngle =
            (2 * Ogre::Degree(Ogre::Math::ASin(orient.x)).valueDegrees());

        if (pitchAngle + shift < 89.5f && pitchAngle + shift > -89.5f) {
            return shift;
        }
        if (pitchAngle > 0) {
            float f = 89.5f - pitchAngle - shift;
            return (f > 0.f) ? f : 0.f;
        } else if (pitchAngle < 0) {
            float f = -89.5 - pitchAngle - shift;
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
}
