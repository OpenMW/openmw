#ifndef GAME_MWRENDER_CAMERA_H
#define GAME_MWRENDER_CAMERA_H

#include <string>

#include "../mwworld/ptr.hpp"

namespace Ogre
{
    class Vector3;
    class Camera;
    class SceneNode;
}

namespace MWRender
{
    class NpcAnimation;

    /// \brief Camera control
    class Camera
    {
        struct CamData {
            float pitch, yaw, offset;
        };

        MWWorld::Ptr mTrackingPtr;

        Ogre::Camera *mCamera;
        Ogre::SceneNode *mCameraNode;

        NpcAnimation *mAnimation;

        bool mFirstPersonView;
        bool mPreviewMode;
        bool mFreeLook;

        struct {
            bool enabled, allowed;
        } mVanity;

        float mHeight, mCameraDistance;
        CamData mMainCam, mPreviewCam;

        bool mDistanceAdjusted;

        /// Updates sound manager listener data
        void updateListener();

        void setLowHeight(bool low = true);

    public:
        Camera(Ogre::Camera *camera);
        ~Camera();

        /// Reset to defaults
        void reset();

        /// Set where the camera is looking at. Uses Morrowind (euler) angles
        /// \param rot Rotation angles in radians
        void rotateCamera(const Ogre::Vector3 &rot, bool adjust);

        float getYaw();
        void setYaw(float angle);

        float getPitch();
        void setPitch(float angle);

        const std::string &getHandle() const;

        /// Attach camera to object
        void attachTo(const MWWorld::Ptr &);

        void toggleViewMode();

        bool toggleVanityMode(bool enable);
        void allowVanityMode(bool allow);

        void togglePreviewMode(bool enable);

        bool isFirstPerson() const
        { return !(mVanity.enabled || mPreviewMode || !mFirstPersonView); }

        void update(float duration);

        /// Set camera distance for current mode. Don't work on 1st person view.
        /// \param adjust Indicates should distance be adjusted or set.
        /// \param override If true new distance will be used as default.
        /// If false, default distance can be restored with setCameraDistance().
        void setCameraDistance(float dist, bool adjust = false, bool override = true);

        /// Restore default camera distance for current mode.
        void setCameraDistance();

        void setAnimation(NpcAnimation *anim);

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
