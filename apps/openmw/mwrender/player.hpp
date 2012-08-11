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

        float controlFlip(float shift = 0.f);

        /// Updates sound manager listener data
        void updateListener();

    public:

        Player (Ogre::Camera *camera, Ogre::SceneNode* mNode);

        /// Set where the player is looking at. Uses Morrowind (euler) angles
        /// \param rot Rotation angles in radians
        /// \return true if player object needs to bo rotated physically
        bool setRotation(const Ogre::Vector3 &rot);

        /// \param rot Rotation angles in radians
        /// \return true if player object needs to bo rotated physically
        bool adjustRotation(const Ogre::Vector3 &rot);

        std::string getHandle() const;

        /// Attach camera to object
        /// \note there is no protection from attaching the same camera to
        /// several different objects
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
