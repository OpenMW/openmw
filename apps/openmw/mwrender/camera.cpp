#include "camera.hpp"

#include <osg/Camera>

#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/settings/settings.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/refdata.hpp"

#include "../mwmechanics/drawstate.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "npcanimation.hpp"

namespace
{

class UpdateRenderCameraCallback : public osg::NodeCallback
{
public:
    UpdateRenderCameraCallback(MWRender::Camera* cam)
        : mCamera(cam)
    {
    }

    virtual void operator()(osg::Node* node, osg::NodeVisitor* nv)
    {
        osg::Camera* cam = static_cast<osg::Camera*>(node);

        // traverse first to update animations, in case the camera is attached to an animated node
        traverse(node, nv);

        mCamera->updateCamera(cam);
    }

private:
    MWRender::Camera* mCamera;
};

}

namespace MWRender
{

    Camera::Camera (osg::Camera* camera)
    : mHeightScale(1.f),
      mCamera(camera),
      mAnimation(nullptr),
      mFirstPersonView(true),
      mPreviewMode(false),
      mNearest(30.f),
      mFurthest(800.f),
      mIsNearest(false),
      mHeight(124.f),
      mBaseCameraDistance(Settings::Manager::getFloat("third person camera distance", "Camera")),
      mVanityToggleQueued(false),
      mVanityToggleQueuedValue(false),
      mViewModeToggleQueued(false),
      mCameraDistance(0.f),
      mFocalPointCurrentOffset(osg::Vec2d()),
      mFocalPointTargetOffset(osg::Vec2d()),
      mFocalPointTransitionSpeedCoef(1.f),
      mPreviousTransitionInfluence(0.f),
      mSmoothedSpeed(0.f),
      mZoomOutWhenMoveCoef(Settings::Manager::getFloat("zoom out when move coef", "Camera")),
      mDynamicCameraDistanceEnabled(false),
      mShowCrosshairInThirdPersonMode(false)
    {
        mVanity.enabled = false;
        mVanity.allowed = true;

        mPreviewCam.pitch = 0.f;
        mPreviewCam.yaw = 0.f;
        mPreviewCam.offset = 400.f;
        mMainCam.pitch = 0.f;
        mMainCam.yaw = 0.f;
        mMainCam.offset = 400.f;

        mCameraDistance = mBaseCameraDistance;

        mUpdateCallback = new UpdateRenderCameraCallback(this);
        mCamera->addUpdateCallback(mUpdateCallback);
    }

    Camera::~Camera()
    {
        mCamera->removeUpdateCallback(mUpdateCallback);
    }

    MWWorld::Ptr Camera::getTrackingPtr() const
    {
        return mTrackingPtr;
    }

    osg::Vec3d Camera::getFocalPoint() const
    {
        const osg::Node* trackNode = mTrackingNode;
        if (!trackNode)
            return osg::Vec3d();
        osg::NodePathList nodepaths = trackNode->getParentalNodePaths();
        if (nodepaths.empty())
            return osg::Vec3d();
        osg::Matrix worldMat = osg::computeLocalToWorld(nodepaths[0]);

        osg::Vec3d position = worldMat.getTrans();
        if (!isFirstPerson())
        {
            position.z() += mHeight * mHeightScale;

            // We subtract 10.f here and add it within focalPointOffset in order to avoid camera clipping through ceiling.
            // Needed because character's head can be a bit higher than collision area.
            position.z() -= 10.f;

            position += getFocalPointOffset() + mFocalPointAdjustment;
        }
        return position;
    }

    osg::Vec3d Camera::getFocalPointOffset() const
    {
        osg::Vec3d offset(0, 0, 10.f);
        if (!mPreviewMode && !mVanity.enabled)
        {
            offset.x() += mFocalPointCurrentOffset.x() * cos(getYaw());
            offset.y() += mFocalPointCurrentOffset.x() * sin(getYaw());
            offset.z() += mFocalPointCurrentOffset.y();
        }
        return offset;
    }

    void Camera::getPosition(osg::Vec3d &focal, osg::Vec3d &camera) const
    {
        focal = getFocalPoint();
        osg::Vec3d offset(0,0,0);
        if (!isFirstPerson())
        {
            osg::Quat orient =  osg::Quat(getPitch(), osg::Vec3d(1,0,0)) * osg::Quat(getYaw(), osg::Vec3d(0,0,1));
            offset = orient * osg::Vec3d(0.f, -mCameraDistance, 0.f);
        }
        camera = focal + offset;
    }

    void Camera::updateCamera(osg::Camera *cam)
    {
        if (mTrackingPtr.isEmpty())
            return;

        osg::Vec3d focal, position;
        getPosition(focal, position);

        osg::Quat orient =  osg::Quat(getPitch(), osg::Vec3d(1,0,0)) * osg::Quat(getYaw(), osg::Vec3d(0,0,1));
        osg::Vec3d forward = orient * osg::Vec3d(0,1,0);
        osg::Vec3d up = orient * osg::Vec3d(0,0,1);

        cam->setViewMatrixAsLookAt(position, position + forward, up);
    }

    void Camera::reset()
    {
        togglePreviewMode(false);
        toggleVanityMode(false);
        if (!mFirstPersonView)
            toggleViewMode();
    }

    void Camera::rotateCamera(float pitch, float yaw, bool adjust)
    {
        if (adjust)
        {
            pitch += getPitch();
            yaw += getYaw();
        }
        setYaw(yaw);
        setPitch(pitch);
    }

    void Camera::attachTo(const MWWorld::Ptr &ptr)
    {
        mTrackingPtr = ptr;
    }

    void Camera::update(float duration, bool paused)
    {
        if (mAnimation->upperBodyReady())
        {
            // Now process the view changes we queued earlier
            if (mVanityToggleQueued)
            {
                toggleVanityMode(mVanityToggleQueuedValue);
                mVanityToggleQueued = false;
            }
            if (mViewModeToggleQueued)
            {

                togglePreviewMode(false);
                toggleViewMode();
                mViewModeToggleQueued = false;
            }
        }

        if (paused)
            return;

        // only show the crosshair in game mode
        MWBase::WindowManager *wm = MWBase::Environment::get().getWindowManager();
        wm->showCrosshair(!wm->isGuiMode() && !mVanity.enabled && !mPreviewMode
                          && (mFirstPersonView || mShowCrosshairInThirdPersonMode));

        if(mVanity.enabled)
        {
            rotateCamera(0.f, osg::DegreesToRadians(3.f * duration), true);
        }

        updateFocalPointOffset(duration);

        float speed = mTrackingPtr.getClass().getSpeed(mTrackingPtr);
        float maxDelta = 300.f * duration;
        mSmoothedSpeed += osg::clampBetween(speed - mSmoothedSpeed, -maxDelta, maxDelta);
    }

    void Camera::setFocalPointTargetOffset(osg::Vec2d v)
    {
        mFocalPointTargetOffset = v;
        mPreviousTransitionSpeed = mFocalPointTransitionSpeed;
        mPreviousTransitionInfluence = 1.0f;
    }

    void Camera::updateFocalPointOffset(float duration)
    {
        if (duration <= 0)
            return;

        osg::Vec2d oldOffset = mFocalPointCurrentOffset;

        if (mPreviousTransitionInfluence > 0)
        {
            mFocalPointCurrentOffset -= mPreviousExtraOffset;
            mPreviousExtraOffset = mPreviousExtraOffset / mPreviousTransitionInfluence + mPreviousTransitionSpeed * duration;
            mPreviousTransitionInfluence =
                std::max(0.f, mPreviousTransitionInfluence - duration * mFocalPointTransitionSpeedCoef);
            mPreviousExtraOffset *= mPreviousTransitionInfluence;
            mFocalPointCurrentOffset += mPreviousExtraOffset;
        }

        osg::Vec2d delta = mFocalPointTargetOffset - mFocalPointCurrentOffset;
        if (delta.length2() > 0)
        {
            float coef = duration * (1.0 + 5.0 / delta.length()) *
                         mFocalPointTransitionSpeedCoef * (1.0f - mPreviousTransitionInfluence);
            mFocalPointCurrentOffset += delta * std::min(coef, 1.0f);
        }
        else
        {
            mPreviousExtraOffset = osg::Vec2d();
            mPreviousTransitionInfluence = 0.f;
        }

        mFocalPointTransitionSpeed = (mFocalPointCurrentOffset - oldOffset) / duration;
    }

    void Camera::toggleViewMode(bool force)
    {
        // Changing the view will stop all playing animations, so if we are playing
        // anything important, queue the view change for later
        if (!mAnimation->upperBodyReady() && !force)
        {
            mViewModeToggleQueued = true;
            return;
        }
        else
            mViewModeToggleQueued = false;

        if (mTrackingPtr.getClass().isActor())
            mTrackingPtr.getClass().getCreatureStats(mTrackingPtr).setSideMovementAngle(0);

        mFirstPersonView = !mFirstPersonView;
        processViewChange();
    }
    
    void Camera::allowVanityMode(bool allow)
    {
        if (!allow && mVanity.enabled)
            toggleVanityMode(false);
        mVanity.allowed = allow;
    }

    bool Camera::toggleVanityMode(bool enable)
    {
        // Changing the view will stop all playing animations, so if we are playing
        // anything important, queue the view change for later
        if (mFirstPersonView && !mAnimation->upperBodyReady())
        {
            mVanityToggleQueued = true;
            mVanityToggleQueuedValue = enable;
            return false;
        }

        if(!mVanity.allowed && enable)
            return false;

        if(mVanity.enabled == enable)
            return true;
        mVanity.enabled = enable;

        processViewChange();

        float offset = mPreviewCam.offset;

        if (mVanity.enabled) {
            setPitch(osg::DegreesToRadians(-30.f));
            mMainCam.offset = mCameraDistance;
        } else {
            offset = mMainCam.offset;
        }

        mCameraDistance = offset;

        return true;
    }

    void Camera::togglePreviewMode(bool enable)
    {
        if (mFirstPersonView && !mAnimation->upperBodyReady())
            return;

        if(mPreviewMode == enable)
            return;

        mPreviewMode = enable;
        processViewChange();

        float offset = mCameraDistance;
        if (mPreviewMode) {
            mMainCam.offset = offset;
            offset = mPreviewCam.offset;
        } else {
            mPreviewCam.offset = offset;
            offset = mMainCam.offset;
        }

        mCameraDistance = offset;
    }

    void Camera::setSneakOffset(float offset)
    {
        mAnimation->setFirstPersonOffset(osg::Vec3f(0,0,-offset));
    }

    float Camera::getYaw() const
    {
        if(mVanity.enabled || mPreviewMode)
            return mPreviewCam.yaw;
        return mMainCam.yaw;
    }

    void Camera::setYaw(float angle)
    {
        if (angle > osg::PI) {
            angle -= osg::PI*2;
        } else if (angle < -osg::PI) {
            angle += osg::PI*2;
        }
        if (mVanity.enabled || mPreviewMode) {
            mPreviewCam.yaw = angle;
        } else {
            mMainCam.yaw = angle;
        }
    }

    float Camera::getPitch() const
    {
        if (mVanity.enabled || mPreviewMode) {
            return mPreviewCam.pitch;
        }
        return mMainCam.pitch;
    }

    void Camera::setPitch(float angle)
    {
        const float epsilon = 0.000001f;
        float limit = osg::PI_2 - epsilon;
        if(mPreviewMode)
            limit /= 2;

        if(angle > limit)
            angle = limit;
        else if(angle < -limit)
            angle = -limit;

        if (mVanity.enabled || mPreviewMode) {
            mPreviewCam.pitch = angle;
        } else {
            mMainCam.pitch = angle;
        }
    }

    float Camera::getCameraDistance() const
    {
        if (isFirstPerson())
            return 0.f;
        return mCameraDistance;
    }

    void Camera::updateBaseCameraDistance(float dist, bool adjust)
    {
        if(mFirstPersonView && !mPreviewMode && !mVanity.enabled)
            return;

        mIsNearest = false;

        if (adjust)
        {
            if (mVanity.enabled || mPreviewMode)
                dist += mCameraDistance;
            else
                dist += std::min(mCameraDistance - getCameraDistanceCorrection(), mBaseCameraDistance);
        }


        if (dist >= mFurthest)
            dist = mFurthest;
        else if (dist <= mNearest)
        {
            dist = mNearest;
            mIsNearest = true;
        }

        if (mVanity.enabled || mPreviewMode)
            mPreviewCam.offset = dist;
        else if (!mFirstPersonView)
        {
            mBaseCameraDistance = dist;
            Settings::Manager::setFloat("third person camera distance", "Camera", dist);
        }
        setCameraDistance();
    }

    void Camera::setCameraDistance(float dist, bool adjust)
    {
        if(mFirstPersonView && !mPreviewMode && !mVanity.enabled)
            return;

        if (adjust) dist += mCameraDistance;

        if (dist >= mFurthest)
            dist = mFurthest;
        else if (dist < 10.f)
            dist = 10.f;
        mCameraDistance = dist;
    }

    float Camera::getCameraDistanceCorrection() const
    {
        if (!mDynamicCameraDistanceEnabled)
            return 0;

        float pitchCorrection = std::max(-getPitch(), 0.f) * 50.f;

        float smoothedSpeedSqr = mSmoothedSpeed * mSmoothedSpeed;
        float speedCorrection = smoothedSpeedSqr / (smoothedSpeedSqr + 300.f*300.f) * mZoomOutWhenMoveCoef;

        return pitchCorrection + speedCorrection;
    }

    void Camera::setCameraDistance()
    {
        if (mVanity.enabled || mPreviewMode)
            mCameraDistance = mPreviewCam.offset;
        else if (!mFirstPersonView)
            mCameraDistance = mBaseCameraDistance + getCameraDistanceCorrection();
        mFocalPointAdjustment = osg::Vec3d();
    }

    void Camera::setAnimation(NpcAnimation *anim)
    {
        mAnimation = anim;

        processViewChange();
    }

    void Camera::processViewChange()
    {
        if(isFirstPerson())
        {
            mAnimation->setViewMode(NpcAnimation::VM_FirstPerson);
            mTrackingNode = mAnimation->getNode("Camera");
            if (!mTrackingNode)
                mTrackingNode = mAnimation->getNode("Head");
            mHeightScale = 1.f;
        }
        else
        {
            mAnimation->setViewMode(NpcAnimation::VM_Normal);
            SceneUtil::PositionAttitudeTransform* transform = mTrackingPtr.getRefData().getBaseNode();
            mTrackingNode = transform;
            if (transform)
                mHeightScale = transform->getScale().z();
            else
                mHeightScale = 1.f;
        }
        rotateCamera(getPitch(), getYaw(), false);
    }

    bool Camera::isVanityOrPreviewModeEnabled() const
    {
        return mPreviewMode || mVanity.enabled;
    }

    bool Camera::isNearest() const
    {
        return mIsNearest;
    }
}
