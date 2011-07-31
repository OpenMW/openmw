#ifndef GAME_MWRENDER_PLAYER_H
#define GAME_MWRENDER_PLAYER_H

#include <iostream>

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
        std::string mHandle;

            public:

                Player (Ogre::Camera *camera, const std::string& handle);

                Ogre::Camera *getCamera() { return mCamera; }

                std::string getHandle() const { std::cout << "mHandle " << mHandle << std::endl; return mHandle; }
    };
}

#endif
