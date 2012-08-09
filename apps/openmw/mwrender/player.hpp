#ifndef GAME_MWRENDER_PLAYER_H
#define GAME_MWRENDER_PLAYER_H

#include <string>


namespace Ogre
{   
    class Vector3;
    class Camera;
    class SceneNode;
}

namespace MWWorld
{
    class Ptr;
}

namespace MWRender
{
    /// \brief Player character rendering and camera control
    class Player
    {
        Ogre::Camera *mCamera;
        Ogre::SceneNode* mNode;

        bool mFirstPersonView;
        bool mVanityModeEnabled;

        void controlFlip();
        void updateListener();

    public:

        Player (Ogre::Camera *camera, Ogre::SceneNode* mNode);

        /// Set where the player is looking at. Uses Morrowind (euler) angles
        bool setRotation(const Ogre::Vector3 &rot);
        bool adjustRotation(const Ogre::Vector3 &rot);

        std::string getHandle() const;

        void attachTo(const MWWorld::Ptr &);

        void toggleViewMode() {
            mFirstPersonView = !mFirstPersonView;
        }

        void toggleVanityMode() {
            mVanityModeEnabled = !mVanityModeEnabled;
        }
    };
}

#endif
