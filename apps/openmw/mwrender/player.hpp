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
    class NpcAnimation;
    /// \brief Player character rendering and camera control
    class Player
    {
        struct CamData {
            float pitch, yaw, offset;
        };

        Ogre::Camera *mCamera;

        Ogre::SceneNode *mPlayerNode;
        Ogre::SceneNode *mCameraNode;

        NpcAnimation *mAnimation;

        bool mFirstPersonView;
        bool mPreviewMode;
        bool mFreeLook;

        struct {
            bool enabled, allowed, forced;
        } mVanity;

        float mHeight, mCameraDistance;
        CamData mMainCam, mPreviewCam;

        bool mDistanceAdjusted;

        /// Updates sound manager listener data
        void updateListener();

        void setLowHeight(bool low = true);

    public:

        Player (Ogre::Camera *camera, Ogre::SceneNode* mNode);
        ~Player();

        /// Set where the player is looking at. Uses Morrowind (euler) angles
        /// \param rot Rotation angles in radians
        /// \return true if player object needs to bo rotated physically
        bool rotate(const Ogre::Vector3 &rot, bool adjust);
        
        void rotateCamera(const Ogre::Vector3 &rot, bool adjust);

        float getYaw();
        void setYaw(float angle);

        float getPitch();
        void setPitch(float angle);

        void compensateYaw(float diff);
        
        std::string getHandle() const;

        /// Attach camera to object
        /// \note there is no protection from attaching the same camera to
        /// several different objects
        void attachTo(const MWWorld::Ptr &);

        void toggleViewMode();

        bool toggleVanityMode(bool enable, bool force = false);
        void allowVanityMode(bool allow);

        void togglePreviewMode(bool enable);

        void update(float duration);

        /// Set camera distance for current mode. Don't work on 1st person view.
        /// \param adjust Indicates should distance be adjusted or set.
        /// \param override If true new distance will be used as default.
        /// If false, default distance can be restored with setCameraDistance().
        void setCameraDistance(float dist, bool adjust = false, bool override = true);

        /// Restore default camera distance for current mode.
        void setCameraDistance();

        void setAnimation(NpcAnimation *anim);
        NpcAnimation *getAnimation() const
        { return mAnimation; }

        void setHeight(float height);
        float getHeight();

        /// Stores player and camera world positions in passed arguments
        /// \return true if camera at the eye-place
        bool getPosition(Ogre::Vector3 &player, Ogre::Vector3 &camera);
        Ogre::Vector3 getPosition();

        void getSightAngles(float &pitch, float &yaw);

        void togglePlayerLooking(bool enable);

        bool isVanityOrPreviewModeEnabled();
    };
}

#endif
