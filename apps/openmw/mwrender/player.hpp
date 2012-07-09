#ifndef GAME_MWRENDER_PLAYER_H
#define GAME_MWRENDER_PLAYER_H

#include <string>

namespace Ogre
{
    class Camera;
    class SceneNode;
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

                /// Set where the player is looking at. Uses Morrowind (euler) angles
                void setRot(float x, float y, float z);

                std::string getHandle() const;
                Ogre::SceneNode* getNode() {return mNode;}
    };
}

#endif
