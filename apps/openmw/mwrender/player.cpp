
#include "player.hpp"

namespace MWRender
{
    Player::Player (Ogre::Camera *camera, Ogre::SceneNode* node)
    : mCamera (camera), mNode (node)
    {}

    void Player::setRot(float x, float y, float z)
    {
            Ogre::SceneNode *sceneNode = mNode;
            Ogre::Node* yawNode = sceneNode->getChildIterator().getNext();
            Ogre::Node* pitchNode = yawNode->getChildIterator().getNext();

            // we are only interested in X and Y rotation

            // Rotate around X axis
            Ogre::Quaternion xr(Ogre::Radian(x), Ogre::Vector3::UNIT_X);

            // Rotate around Y axis
            Ogre::Quaternion yr(Ogre::Radian(-z), Ogre::Vector3::UNIT_Y);

            pitchNode->setOrientation(xr);
            yawNode->setOrientation(yr);
    }
}
