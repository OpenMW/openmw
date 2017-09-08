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

    enum CameraView {
        // individual Views
        CameraView_FirstPerson                        = 0x1, // 0b0001
        CameraView_ThirdPersonCenter                  = 0x2, // 0b0010
        CameraView_ThirdPersonOverShoulder            = 0x4, // 0b0100
        CameraView_ThirdPersonOverTheShoulderRanged   = 0x8, // 0b1000
        CameraView_NumCameraViews                     = 4,
        // ViewSets
        // Match any of ThirdPersonCenter and ThirdPersonOverShoulder
        CameraView_AnyThirdPersonNonCombat            = 0x6, // 0b0110
        // Only ThirdPersonOverTheShoulderRanged right now
        CameraView_AnyThirdPersonCombat               = 0x8, // 0b1000
        // Any CameraView_ThirdPerson* option
        CameraView_AnyThirdPerson                     = 0xe, // 0b1110
        // Any CameraView that should allow crosshair to be visibile
        CameraView_ShowCrosshair                      = 0xd, // 0b1101
    };
    class NpcAnimation;

    /// \brief Camera control
    class Camera
    {
        // The reset camera distances for the various views
        static const float sResetThirdPersonCameraDistance;

        struct CamData {
            float pitch, yaw, offset;
        };

        MWWorld::Ptr mTrackingPtr;
        osg::ref_ptr<const osg::Node> mTrackingNode;
        float mHeightScale;

        osg::ref_ptr<osg::Camera> mCamera;

        NpcAnimation *mAnimation;

        CameraView mCameraView;
        CameraView mActiveThirdPersonView;
        bool mPreviewMode;
        bool mFreeLook;
        float mNearest;
        float mFurthest;
        bool mIsNearest;

        struct {
            bool enabled, allowed;
        } mVanity;

        float mHeight, mMaxCameraDistance;

        CamData mMainCam, mPreviewCam;

        bool mVanityToggleQueued;
        bool mVanityToggleQueuedValue;
        bool mViewModeToggleQueued;

        /// FIXME: Make this into a 3d vector?
        float mCameraDistance;
        float mCameraXOffsetFromCenter;
        float mCameraZOffsetFromCenter;

        bool mCameraRotationDisjoint;

        osg::ref_ptr<osg::NodeCallback> mUpdateCallback;

    public:
        Camera(osg::Camera* camera);
        ~Camera();

        MWWorld::Ptr getTrackingPtr() const;

        /// Update the view matrix of \a cam
        void updateCamera(osg::Camera* cam);

        /// Reset to defaults
        void reset();

        /// Set where the camera is looking at. Uses Morrowind (euler) angles
        /// \param rot Rotation angles in radians
        void rotateCamera(float pitch, float yaw, bool adjust);

        float getYaw();
        void setYaw(float angle);

        float getPitch();
        void setPitch(float angle);

        /// Attach camera to object
        void attachTo(const MWWorld::Ptr &);

        /// @param Force view mode switch, even if currently not allowed by the animation.
        void toggleViewMode(bool force=false);

        /// Change to Camera View Mode specified.
        /// @param newCameraView The camera view to switch to.
        void changeToViewMode(CameraView newCameraView);

        bool toggleVanityMode(bool enable);
        void allowVanityMode(bool allow);

        /// @note this may be ignored if an important animation is currently playing
        void togglePreviewMode(bool enable);

        /// This will toggle between current active third person view and archery/spell third person view
        /// when over the shoulder third person is enabled.
        void toggleThirdPersonOverShouldRangedCamera();
        void setThirdPersonOverShouldRangedCamera(bool set);

        /// \brief Lowers the camera for sneak.
        void setSneakOffset(float offset);

        bool isFirstPerson() const
        { return !(mVanity.enabled || mPreviewMode || (mCameraView != CameraView_FirstPerson)); }

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

        osg::Vec3d getFocalPoint();

        /// Stores focal and camera world positions in passed arguments
        void getPosition(osg::Vec3f &focal, osg::Vec3f &camera);

        void togglePlayerLooking(bool enable);

        bool isVanityOrPreviewModeEnabled();

        bool isNearest();
    };
}

#endif
