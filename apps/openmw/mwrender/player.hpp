#ifndef GAME_MWRENDER_PLAYER_H
#define GAME_MWRENDER_PLAYER_H

#include <iostream>
#include <Ogre.h>

namespace Ogre
{
    class Camera;
}

namespace MWRender
{
    /// \brief Player character rendering and camera control
    class Player
    {
        Ogre::Camera *mCamera;
        Ogre::SceneNode* mNode;

            public:

                Player (Ogre::Camera *camera, Ogre::SceneNode* mNode);

                Ogre::Camera *getCamera() { return mCamera; }

                std::string getHandle() const { return mNode->getName(); }
                Ogre::SceneNode* getNode() {return mNode;}
    };
}

#endif
