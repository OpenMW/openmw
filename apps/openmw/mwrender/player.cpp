#include "player.hpp"

#include <OgreSceneNode.h>
#include <OgreCamera.h>

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
        Ogre::Quaternion xr(Ogre::Radian(rot.x), Ogre::Vector3::UNIT_X);

        // Rotate around Y axis
        Ogre::Quaternion yr(Ogre::Radian(-rot.z), Ogre::Vector3::UNIT_Y);

        pitchNode->setOrientation(xr);
        yawNode->setOrientation(yr);

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
        yawNode->yaw(Ogre::Degree(rot.z));

        return !mVanityModeEnabled;
    }
}
