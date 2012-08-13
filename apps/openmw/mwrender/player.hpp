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
        struct CamData {
            float pitch, yaw, offset;
        };

        Ogre::Camera *mCamera;

        Ogre::SceneNode *mPlayerNode;
        Ogre::SceneNode *mCameraNode;
        Ogre::SceneNode *mVanityNode;

        bool mFirstPersonView;
        bool mVanityMode;
        bool mPreviewMode;

        float mHeight;
        CamData mMainCam, mPreviewCam;

        float mTimeIdle;
        int mUpdates;

        /// Updates sound manager listener data
        void updateListener();

        void rotateCamera(const Ogre::Vector3 &rot, bool adjust);

        float getYaw();
        void setYaw(float angle);

        float getPitch();
        void setPitch(float angle);

    public:

        Player (Ogre::Camera *camera, Ogre::SceneNode* mNode);

        /// Set where the player is looking at. Uses Morrowind (euler) angles
        /// \param rot Rotation angles in radians
        /// \return true if player object needs to bo rotated physically
        bool rotate(const Ogre::Vector3 &rot, bool adjust);

        std::string getHandle() const;

        /// Attach camera to object
        /// \note there is no protection from attaching the same camera to
        /// several different objects
        void attachTo(const MWWorld::Ptr &);

        void toggleViewMode();

        void toggleVanityMode(bool enable, bool force = false);

        void togglePreviewMode(bool enable);

        void update(float duration);
    };
}

#endif
