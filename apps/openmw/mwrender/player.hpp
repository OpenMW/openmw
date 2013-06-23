#ifndef GAME_MWRENDER_PLAYER_H
#define GAME_MWRENDER_PLAYER_H

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

            public:

                Player (Ogre::Camera *camera);

                Ogre::Camera *getCamera() { return mCamera; }
    };
}

#endif
