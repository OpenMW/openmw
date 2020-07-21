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
#include "../mwmechanics/movement.hpp"
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
      mMode(Mode::Normal),
      mVanityAllowed(true),
      mStandingPreviewAllowed(Settings::Manager::getBool("preview if stand still", "Camera")),
      mDeferredRotationAllowed(Settings::Manager::getBool("deferred preview rotation", "Camera")),
      mNearest(30.f),
      mFurthest(800.f),
      mIsNearest(false),
      mHeight(124.f),
      mBaseCameraDistance(Settings::Manager::getFloat("third person camera distance", "Camera")),
      mVanityToggleQueued(false),
      mVanityToggleQueuedValue(false),
      mViewModeToggleQueued(false),
      mCameraDistance(0.f),
      mMaxNextCameraDistance(800.f),
      mFocalPointCurrentOffset(osg::Vec2d()),
      mFocalPointTargetOffset(osg::Vec2d()),
      mFocalPointTransitionSpeedCoef(1.f),
      mSkipFocalPointTransition(true),
      mPreviousTransitionInfluence(0.f),
      mSmoothedSpeed(0.f),
      mZoomOutWhenMoveCoef(Settings::Manager::getFloat("zoom out when move coef", "Camera")),
      mDynamicCameraDistanceEnabled(false),
      mShowCrosshairInThirdPersonMode(false),
      mDeferredRotation(osg::Vec3f()),
      mDeferredRotationDisabled(false)
    {
        mCameraDistance = mBaseCameraDistance;

        mUpdateCallback = new UpdateRenderCameraCallback(this);
        mCamera->addUpdateCallback(mUpdateCallback);
    }

    Camera::~Camera()
    {
        mCamera->removeUpdateCallback(mUpdateCallback);
    }

    osg::Vec3d Camera::getFocalPoint() const
    {
        if (!mTrackingNode)
            return osg::Vec3d();
        osg::NodePathList nodepaths = mTrackingNode->getParentalNodePaths();
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
        offset.x() += mFocalPointCurrentOffset.x() * cos(getYaw());
        offset.y() += mFocalPointCurrentOffset.x() * sin(getYaw());
        offset.z() += mFocalPointCurrentOffset.y();
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
        wm->showCrosshair(!wm->isGuiMode() && mMode != Mode::Preview && mMode != Mode::Vanity
                          && (mFirstPersonView || mShowCrosshairInThirdPersonMode));

        if(mMode == Mode::Vanity)
            rotateCamera(0.f, osg::DegreesToRadians(3.f * duration), true);

        updateFocalPointOffset(duration);

        float speed = mTrackingPtr.getClass().getSpeed(mTrackingPtr);
        speed /= (1.f + speed / 500.f);
        float maxDelta = 300.f * duration;
        mSmoothedSpeed += osg::clampBetween(speed - mSmoothedSpeed, -maxDelta, maxDelta);

        mMaxNextCameraDistance = mCameraDistance + duration * (100.f + mBaseCameraDistance);
        updateStandingPreviewMode();
    }

    void Camera::updateStandingPreviewMode()
    {
        if (!mStandingPreviewAllowed)
            return;
        float speed = mTrackingPtr.getClass().getSpeed(mTrackingPtr);
        bool combat = mTrackingPtr.getClass().isActor() &&
                      mTrackingPtr.getClass().getCreatureStats(mTrackingPtr).getDrawState() != MWMechanics::DrawState_Nothing;
        bool standingStill = speed == 0 && !combat && !mFirstPersonView;
        if (!standingStill && mMode == Mode::StandingPreview)
        {
            mMode = Mode::Normal;
            calculateDeferredRotation();
        }
        else if (standingStill && mMode == Mode::Normal)
            mMode = Mode::StandingPreview;
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

        if (mSkipFocalPointTransition)
        {
            mSkipFocalPointTransition = false;
            mPreviousExtraOffset = osg::Vec2d();
            mPreviousTransitionInfluence = 0.f;
            mFocalPointCurrentOffset = mFocalPointTargetOffset;
        }

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
        updateStandingPreviewMode();
        instantTransition();
        processViewChange();
    }

    void Camera::allowVanityMode(bool allow)
    {
        if (!allow && mMode == Mode::Vanity)
        {
            disableDeferredPreviewRotation();
            toggleVanityMode(false);
        }
        mVanityAllowed = allow;
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

        if (!mVanityAllowed && enable)
            return false;

        if ((mMode == Mode::Vanity) == enable)
            return true;
        mMode = enable ? Mode::Vanity : Mode::Normal;
        if (!mDeferredRotationAllowed)
            disableDeferredPreviewRotation();
        if (!enable)
            calculateDeferredRotation();

        processViewChange();
        return true;
    }

    void Camera::togglePreviewMode(bool enable)
    {
        if (mFirstPersonView && !mAnimation->upperBodyReady())
            return;

        if((mMode == Mode::Preview) == enable)
            return;

        mMode = enable ? Mode::Preview : Mode::Normal;
        if (mMode == Mode::Normal)
            updateStandingPreviewMode();
        else if (mFirstPersonView)
            instantTransition();
        if (mMode == Mode::Normal)
        {
            if (!mDeferredRotationAllowed)
                disableDeferredPreviewRotation();
            calculateDeferredRotation();
        }
        processViewChange();
    }

    void Camera::setSneakOffset(float offset)
    {
        mAnimation->setFirstPersonOffset(osg::Vec3f(0,0,-offset));
    }

    void Camera::setYaw(float angle)
    {
        if (angle > osg::PI) {
            angle -= osg::PI*2;
        } else if (angle < -osg::PI) {
            angle += osg::PI*2;
        }
        mYaw = angle;
    }

    void Camera::setPitch(float angle)
    {
        const float epsilon = 0.000001f;
        float limit = osg::PI_2 - epsilon;
        mPitch = osg::clampBetween(angle, -limit, limit);
    }

    float Camera::getCameraDistance() const
    {
        if (isFirstPerson())
            return 0.f;
        return mCameraDistance;
    }

    void Camera::updateBaseCameraDistance(float dist, bool adjust)
    {
        if (isFirstPerson())
            return;

        if (adjust)
            dist += std::min(mCameraDistance - getCameraDistanceCorrection(), mBaseCameraDistance);

        mIsNearest = dist <= mNearest;
        mBaseCameraDistance = osg::clampBetween(dist, mNearest, mFurthest);
        Settings::Manager::setFloat("third person camera distance", "Camera", mBaseCameraDistance);
        setCameraDistance();
    }

    void Camera::setCameraDistance(float dist, bool adjust)
    {
        if (isFirstPerson())
            return;
        if (adjust)
            dist += mCameraDistance;
        mCameraDistance = osg::clampBetween(dist, 10.f, mFurthest);
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
        mFocalPointAdjustment = osg::Vec3d();
        if (isFirstPerson())
            return;
        mCameraDistance = mBaseCameraDistance + getCameraDistanceCorrection();
        if (mDynamicCameraDistanceEnabled)
            mCameraDistance = std::min(mCameraDistance, mMaxNextCameraDistance);
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

    void Camera::applyDeferredPreviewRotationToPlayer(float dt)
    {
        if (isVanityOrPreviewModeEnabled() || mTrackingPtr.isEmpty())
            return;

        osg::Vec3f rot = mDeferredRotation;
        float delta = rot.normalize();
        delta = std::min(delta, (delta + 1.f) * 3 * dt);
        rot *= delta;
        mDeferredRotation -= rot;

        if (mDeferredRotationDisabled)
        {
            mDeferredRotationDisabled = delta > 0.0001;
            rotateCameraToTrackingPtr();
            return;
        }

        auto& movement = mTrackingPtr.getClass().getMovementSettings(mTrackingPtr);
        movement.mRotation[0] += rot.x();
        movement.mRotation[1] += rot.y();
        movement.mRotation[2] += rot.z();
        if (std::abs(mDeferredRotation.z()) > 0.0001)
        {
            float s = std::sin(mDeferredRotation.z());
            float c = std::cos(mDeferredRotation.z());
            float x = movement.mPosition[0];
            float y = movement.mPosition[1];
            movement.mPosition[0] = x *  c + y * s;
            movement.mPosition[1] = x * -s + y * c;
        }
    }

    void Camera::rotateCameraToTrackingPtr()
    {
        setPitch(-mTrackingPtr.getRefData().getPosition().rot[0] - mDeferredRotation.x());
        setYaw(-mTrackingPtr.getRefData().getPosition().rot[2] - mDeferredRotation.z());
    }

    void Camera::instantTransition()
    {
        mSkipFocalPointTransition = true;
        mDeferredRotationDisabled = false;
        mDeferredRotation = osg::Vec3f();
        rotateCameraToTrackingPtr();
    }

    void Camera::calculateDeferredRotation()
    {
        MWWorld::Ptr ptr = mTrackingPtr;
        if (isVanityOrPreviewModeEnabled() || ptr.isEmpty())
            return;
        if (mFirstPersonView)
        {
            instantTransition();
            return;
        }

        mDeferredRotation.x() = -ptr.getRefData().getPosition().rot[0] - mPitch;
        mDeferredRotation.z() = -ptr.getRefData().getPosition().rot[2] - mYaw;
        if (mDeferredRotation.x() > osg::PI)
            mDeferredRotation.x() -= 2 * osg::PI;
        if (mDeferredRotation.x() < -osg::PI)
            mDeferredRotation.x() += 2 * osg::PI;
        if (mDeferredRotation.z() > osg::PI)
            mDeferredRotation.z() -= 2 * osg::PI;
        if (mDeferredRotation.z() < -osg::PI)
            mDeferredRotation.z() += 2 * osg::PI;
    }

}
