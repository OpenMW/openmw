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
        float mNearest;
        float mFurthest;
        bool mIsNearest;
        bool mIsFurthest;

        struct {
            bool enabled, allowed;
        } mVanity;

        float mHeight, mCameraDistance;
        CamData mMainCam, mPreviewCam;

        bool mDistanceAdjusted;

        bool mVanityToggleQueued;
        bool mViewModeToggleQueued;

        /// Updates sound manager listener data
        void updateListener();

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

        /// @note this may be ignored if an important animation is currently playing
        void togglePreviewMode(bool enable);

        /// \brief Lowers the camera for sneak.
        /// As animation is tied to the camera, this needs
        /// to be set each frame after the animation is
        /// applied.
        void setSneakOffset(float offset);

        bool isFirstPerson() const
        { return !(mVanity.enabled || mPreviewMode || !mFirstPersonView); }

        void processViewChange();

        void update(float duration, bool paused=false);

        /// Set camera distance for current mode. Don't work on 1st person view.
        /// \param adjust Indicates should distance be adjusted or set.
        /// \param override If true new distance will be used as default.
        /// If false, default distance can be restored with setCameraDistance().
        void setCameraDistance(float dist, bool adjust = false, bool override = true);

        /// Restore default camera distance for current mode.
        void setCameraDistance();

        float getCameraDistance() const;

        void setAnimation(NpcAnimation *anim);

        /// Stores focal and camera world positions in passed arguments
        void getPosition(Ogre::Vector3 &focal, Ogre::Vector3 &camera);

        void togglePlayerLooking(bool enable);

        bool isVanityOrPreviewModeEnabled();

        bool isNearest();

        bool isFurthest();
    };
}

#endif
