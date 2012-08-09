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
        Ogre::Quaternion xr(Ogre::Degree(rot.x), Ogre::Vector3::UNIT_X);

        // Rotate around Y axis
        Ogre::Quaternion yr(Ogre::Degree(-rot.z), Ogre::Vector3::UNIT_Y);

        pitchNode->setOrientation(xr);
        yawNode->setOrientation(yr);

        controlFlip();
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

        pitchNode->pitch(Ogre::Degree(rot.x));
        yawNode->yaw(Ogre::Degree(-rot.z));

        controlFlip();
        updateListener();

        return !mVanityModeEnabled;
    }

    void Player::controlFlip()
    {
        Ogre::SceneNode *pitchNode = mCamera->getParentSceneNode();
        Ogre::Quaternion orient = pitchNode->getOrientation();

        float pitchAngle =
            (2 * Ogre::Degree(Ogre::Math::ACos(orient.w)).valueDegrees());

        // Limit the pitch between -90 degress and +90 degrees, Quake3-style.
        if (pitchAngle > 90.0f)
        {
            Ogre::Real sqrt = Ogre::Math::Sqrt(0.5f);
            if (orient.x > 0) {
                // Set orientation to 90 degrees on X-axis.
                pitchNode->setOrientation(Ogre::Quaternion(sqrt, sqrt, 0, 0));
            } else if (orient.x < 0) {
                // Sets orientation to -90 degrees on X-axis.
                pitchNode->setOrientation(Ogre::Quaternion(sqrt, -sqrt, 0, 0));
            }
        }
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
