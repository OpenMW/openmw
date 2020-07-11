#ifndef GAME_MWRENDER_CAMERA_H
#define GAME_MWRENDER_CAMERA_H

#include <string>

#include <osg/ref_ptr>
#include <osg/Vec3>
#include <osg/Vec3d>

#include "../mwworld/ptr.hpp"

namespace osg
{
    class Camera;
    class NodeCallback;
    class Node;
}

namespace MWRender
{
    class NpcAnimation;

    /// \brief Camera control
    class Camera
    {
    public:
        enum class ThirdPersonViewMode {Standard, OverShoulder};

    private:
        struct CamData {
            float pitch, yaw, offset;
        };

        MWWorld::Ptr mTrackingPtr;
        osg::ref_ptr<const osg::Node> mTrackingNode;
        float mHeightScale;

        osg::ref_ptr<osg::Camera> mCamera;

        NpcAnimation *mAnimation;

        bool mFirstPersonView;
        bool mPreviewMode;
        float mNearest;
        float mFurthest;
        bool mIsNearest;

        struct {
            bool enabled, allowed;
        } mVanity;

        float mHeight, mBaseCameraDistance;
        CamData mMainCam, mPreviewCam;

        bool mVanityToggleQueued;
        bool mVanityToggleQueuedValue;
        bool mViewModeToggleQueued;

        float mCameraDistance;

        ThirdPersonViewMode mThirdPersonMode;
        osg::Vec2f mOverShoulderOffset;
        osg::Vec3d mFocalPointAdjustment;

        // Makes sense only if mThirdPersonMode is OverShoulder. Can be in range [0, 1].
        // Used for smooth transition from non-combat camera position (0) to combat camera position (1).
        float mSmoothTransitionToCombatMode;
        void updateSmoothTransitionToCombatMode(float duration);
        float getCameraDistanceCorrection() const;

        osg::ref_ptr<osg::NodeCallback> mUpdateCallback;

    public:
        Camera(osg::Camera* camera);
        ~Camera();

        MWWorld::Ptr getTrackingPtr() const;

        void setThirdPersonViewMode(ThirdPersonViewMode mode) { mThirdPersonMode = mode; }
        void setOverShoulderOffset(float horizontal, float vertical);

        /// Update the view matrix of \a cam
        void updateCamera(osg::Camera* cam);

        /// Reset to defaults
        void reset();

        /// Set where the camera is looking at. Uses Morrowind (euler) angles
        /// \param rot Rotation angles in radians
        void rotateCamera(float pitch, float yaw, bool adjust);

        float getYaw() const;
        void setYaw(float angle);

        float getPitch() const;
        void setPitch(float angle);

        /// Attach camera to object
        void attachTo(const MWWorld::Ptr &);

        /// @param Force view mode switch, even if currently not allowed by the animation.
        void toggleViewMode(bool force=false);

        bool toggleVanityMode(bool enable);
        void allowVanityMode(bool allow);

        /// @note this may be ignored if an important animation is currently playing
        void togglePreviewMode(bool enable);

        /// \brief Lowers the camera for sneak.
        void setSneakOffset(float offset);

        bool isFirstPerson() const
        { return !(mVanity.enabled || mPreviewMode || !mFirstPersonView); }

        void processViewChange();

        void update(float duration, bool paused=false);

        /// Set base camera distance for current mode. Don't work on 1st person view.
        /// \param adjust Indicates should distance be adjusted or set.
        void updateBaseCameraDistance(float dist, bool adjust = false);

        /// Set camera distance for current mode. Don't work on 1st person view.
        /// \param adjust Indicates should distance be adjusted or set.
        /// Default distance can be restored with setCameraDistance().
        void setCameraDistance(float dist, bool adjust = false);

        /// Restore default camera distance and offset for current mode.
        void setCameraDistance();

        float getCameraDistance() const;

        void setAnimation(NpcAnimation *anim);

        osg::Vec3d getFocalPoint() const;
        osg::Vec3d getFocalPointOffset() const;
        void adjustFocalPoint(osg::Vec3d adjustment) { mFocalPointAdjustment = adjustment; }

        /// Stores focal and camera world positions in passed arguments
        void getPosition(osg::Vec3d &focal, osg::Vec3d &camera) const;

        bool isVanityOrPreviewModeEnabled() const;

        bool isNearest() const;
    };
}

#endif
