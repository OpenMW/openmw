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
        enum class Mode { Normal, Vanity, Preview, StandingPreview };

    private:
        MWWorld::Ptr mTrackingPtr;
        osg::ref_ptr<const osg::Node> mTrackingNode;
        float mHeightScale;

        osg::ref_ptr<osg::Camera> mCamera;

        NpcAnimation *mAnimation;

        bool mFirstPersonView;
        Mode mMode;
        bool mVanityAllowed;
        bool mStandingPreviewAllowed;
        bool mDeferredRotationAllowed;

        float mNearest;
        float mFurthest;
        bool mIsNearest;

        float mHeight, mBaseCameraDistance;
        float mPitch, mYaw;

        bool mVanityToggleQueued;
        bool mVanityToggleQueuedValue;
        bool mViewModeToggleQueued;

        float mCameraDistance;
        float mMaxNextCameraDistance;

        osg::Vec3d mFocalPointAdjustment;
        osg::Vec2d mFocalPointCurrentOffset;
        osg::Vec2d mFocalPointTargetOffset;
        float mFocalPointTransitionSpeedCoef;
        bool mSkipFocalPointTransition;

        // This fields are used to make focal point transition smooth if previous transition was not finished.
        float mPreviousTransitionInfluence;
        osg::Vec2d mFocalPointTransitionSpeed;
        osg::Vec2d mPreviousTransitionSpeed;
        osg::Vec2d mPreviousExtraOffset;

        float mSmoothedSpeed;
        float mZoomOutWhenMoveCoef;
        bool mDynamicCameraDistanceEnabled;
        bool mShowCrosshairInThirdPersonMode;

        void updateFocalPointOffset(float duration);
        float getCameraDistanceCorrection() const;

        osg::ref_ptr<osg::NodeCallback> mUpdateCallback;

        // Used to rotate player to the direction of view after exiting preview or vanity mode.
        osg::Vec3f mDeferredRotation;
        bool mDeferredRotationDisabled;
        void calculateDeferredRotation();
        void updateStandingPreviewMode();

    public:
        Camera(osg::Camera* camera);
        ~Camera();

        /// Attach camera to object
        void attachTo(const MWWorld::Ptr &ptr) { mTrackingPtr = ptr; }
        MWWorld::Ptr getTrackingPtr() const { return mTrackingPtr; }

        void setFocalPointTransitionSpeed(float v) { mFocalPointTransitionSpeedCoef = v; }
        void setFocalPointTargetOffset(osg::Vec2d v);
        void instantTransition();
        void enableDynamicCameraDistance(bool v) { mDynamicCameraDistanceEnabled = v; }
        void enableCrosshairInThirdPersonMode(bool v) { mShowCrosshairInThirdPersonMode = v; }

        /// Update the view matrix of \a cam
        void updateCamera(osg::Camera* cam);

        /// Reset to defaults
        void reset();

        /// Set where the camera is looking at. Uses Morrowind (euler) angles
        /// \param rot Rotation angles in radians
        void rotateCamera(float pitch, float yaw, bool adjust);
        void rotateCameraToTrackingPtr();

        float getYaw() const { return mYaw; }
        void setYaw(float angle);

        float getPitch() const { return mPitch; }
        void setPitch(float angle);

        /// @param Force view mode switch, even if currently not allowed by the animation.
        void toggleViewMode(bool force=false);

        bool toggleVanityMode(bool enable);
        void allowVanityMode(bool allow);

        /// @note this may be ignored if an important animation is currently playing
        void togglePreviewMode(bool enable);

        void applyDeferredPreviewRotationToPlayer(float dt);
        void disableDeferredPreviewRotation() { mDeferredRotationDisabled = true; }

        /// \brief Lowers the camera for sneak.
        void setSneakOffset(float offset);

        bool isFirstPerson() const { return mFirstPersonView && mMode == Mode::Normal; }

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

        bool isVanityOrPreviewModeEnabled() const { return mMode != Mode::Normal; }
        Mode getMode() const { return mMode; }

        bool isNearest() const { return mIsNearest; }
    };
}

#endif
