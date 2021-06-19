#include "camera.hpp"

#include <osg/Camera>

#include <components/misc/mathutil.hpp>
#include <components/sceneutil/positionattitudetransform.hpp>
#include <components/settings/settings.hpp>
#include <components/sceneutil/nodecallback.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"
#include "../mwbase/world.hpp"

#include "../mwworld/class.hpp"
#include "../mwworld/ptr.hpp"
#include "../mwworld/refdata.hpp"

#include "../mwmechanics/drawstate.hpp"
#include "../mwmechanics/movement.hpp"
#include "../mwmechanics/npcstats.hpp"

#include "../mwphysics/raycasting.hpp"

#include "npcanimation.hpp"

namespace
{

class UpdateRenderCameraCallback : public SceneUtil::NodeCallback<UpdateRenderCameraCallback, osg::Camera*>
{
public:
    UpdateRenderCameraCallback(MWRender::Camera* cam)
        : mCamera(cam)
    {
    }

    void operator()(osg::Camera* cam, osg::NodeVisitor* nv)
    {
        // traverse first to update animations, in case the camera is attached to an animated node
        traverse(cam, nv);

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
      mMode(Mode::FirstPerson),
      mVanityAllowed(true),
      mStandingPreviewAllowed(Settings::Manager::getBool("preview if stand still", "Camera")),
      mDeferredRotationAllowed(Settings::Manager::getBool("deferred preview rotation", "Camera")),
      mNearest(30.f),
      mFurthest(800.f),
      mIsNearest(false),
      mProcessViewChange(false),
      mHeight(124.f),
      mBaseCameraDistance(Settings::Manager::getFloat("third person camera distance", "Camera")),
      mPitch(0.f),
      mYaw(0.f),
      mRoll(0.f),
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
      mHeadBobbingEnabled(Settings::Manager::getBool("head bobbing", "Camera")),
      mHeadBobbingOffset(0.f),
      mHeadBobbingWeight(0.f),
      mTotalMovement(0.f),
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

    osg::Vec3d Camera::getTrackingNodePosition() const
    {
        if (!mTrackingNode)
            return osg::Vec3d();
        osg::NodePathList nodepaths = mTrackingNode->getParentalNodePaths();
        if (nodepaths.empty())
            return osg::Vec3d();
        osg::Matrix worldMat = osg::computeLocalToWorld(nodepaths[0]);
        return worldMat.getTrans();
    }

    osg::Vec3d Camera::getThirdPersonBasePosition() const
    {
        osg::Vec3d position = getTrackingNodePosition();
        position.z() += mHeight * mHeightScale;

        // We subtract 10.f here and add it within focalPointOffset in order to avoid camera clipping through ceiling.
        // Needed because character's head can be a bit higher than collision area.
        position.z() -= 10.f;

        return position;
    }

    osg::Vec3d Camera::getFocalPointOffset() const
    {
        osg::Vec3d offset(0, 0, 10.f);
        offset.x() += mFocalPointCurrentOffset.x() * cos(mYaw);
        offset.y() += mFocalPointCurrentOffset.x() * sin(mYaw);
        offset.z() += mFocalPointCurrentOffset.y();
        return offset;
    }

    void Camera::updateCamera(osg::Camera *cam)
    {
        osg::Quat orient = osg::Quat(mRoll, osg::Vec3d(0, 1, 0)) * osg::Quat(mPitch, osg::Vec3d(1, 0, 0)) * osg::Quat(mYaw, osg::Vec3d(0, 0, 1));
        osg::Vec3d forward = orient * osg::Vec3d(0,1,0);
        osg::Vec3d up = orient * osg::Vec3d(0,0,1);

        cam->setViewMatrixAsLookAt(mPosition, mPosition + forward, up);
    }

    void Camera::updateHeadBobbing(float duration) {
        static const float doubleStepLength = Settings::Manager::getFloat("head bobbing step", "Camera") * 2;
        static const float stepHeight = Settings::Manager::getFloat("head bobbing height", "Camera");
        static const float maxRoll = osg::DegreesToRadians(Settings::Manager::getFloat("head bobbing roll", "Camera"));

        if (MWBase::Environment::get().getWorld()->isOnGround(mTrackingPtr))
            mHeadBobbingWeight = std::min(mHeadBobbingWeight + duration * 5, 1.f);
        else
            mHeadBobbingWeight = std::max(mHeadBobbingWeight - duration * 5, 0.f);

        float doubleStepState = mTotalMovement / doubleStepLength - std::floor(mTotalMovement / doubleStepLength); // from 0 to 1 during 2 steps
        float stepState = std::abs(doubleStepState * 4 - 2) - 1; // from -1 to 1 on even steps and from 1 to -1 on odd steps
        float effect = (1 - std::cos(stepState * osg::DegreesToRadians(30.f))) * 7.5f; // range from 0 to 1
        float coef = std::min(mSmoothedSpeed / 300.f, 1.f) * mHeadBobbingWeight;
        mHeadBobbingOffset = (0.5f - effect) * coef * stepHeight; // range from -stepHeight/2 to stepHeight/2
        mRoll = osg::sign(stepState) * effect * coef * maxRoll; // range from -maxRoll to maxRoll
    }

    void Camera::update(float duration, bool paused)
    {
        if (mQueuedMode && mAnimation->upperBodyReady())
            setMode(*mQueuedMode);
        if (mProcessViewChange)
            processViewChange();

        if (paused)
            return;

        // only show the crosshair in game mode
        MWBase::WindowManager *wm = MWBase::Environment::get().getWindowManager();
        wm->showCrosshair(!wm->isGuiMode() && mMode != Mode::Preview && mMode != Mode::Vanity
                          && (mFirstPersonView || mShowCrosshairInThirdPersonMode));

        if(mMode == Mode::Vanity)
            setYaw(mYaw + osg::DegreesToRadians(3.f) * duration);

        if (mMode == Mode::FirstPerson && mHeadBobbingEnabled)
            updateHeadBobbing(duration);
        else
            mRoll = mHeadBobbingOffset = 0;

        updateFocalPointOffset(duration);
        updatePosition();

        float speed = mTrackingPtr.getClass().getCurrentSpeed(mTrackingPtr);
        mTotalMovement += speed * duration;
        speed /= (1.f + speed / 500.f);
        float maxDelta = 300.f * duration;
        mSmoothedSpeed += std::clamp(speed - mSmoothedSpeed, -maxDelta, maxDelta);

        mMaxNextCameraDistance = mCameraDistance + duration * (100.f + mBaseCameraDistance);
        updateStandingPreviewMode();
    }

    void Camera::updatePosition()
    {
        if (mMode == Mode::Static)
            return;
        if (mMode == Mode::FirstPerson)
        {
            mPosition = getTrackingNodePosition();
            mPosition.z() += mHeadBobbingOffset;
            return;
        }

        constexpr float cameraObstacleLimit = 5.0f;
        constexpr float focalObstacleLimit = 10.f;

        const auto* rayCasting = MWBase::Environment::get().getWorld()->getRayCasting();
        constexpr int collisionType = (MWPhysics::CollisionType::CollisionType_Default & ~MWPhysics::CollisionType::CollisionType_Actor);

        // Adjust focal point to prevent clipping.
        osg::Vec3d focalOffset = getFocalPointOffset();
        osg::Vec3d focal = getThirdPersonBasePosition() + focalOffset;
        float offsetLen = focalOffset.length();
        if (offsetLen > 0)
        {
            MWPhysics::RayCastingResult result = rayCasting->castSphere(focal - focalOffset, focal, focalObstacleLimit, collisionType);
            if (result.mHit)
            {
                double adjustmentCoef = -(result.mHitPos + result.mHitNormal * focalObstacleLimit - focal).length() / offsetLen;
                focal += focalOffset * std::max(-1.0, adjustmentCoef);
            }
        }

        // Calculate offset from focal point.
        mCameraDistance = mBaseCameraDistance + getCameraDistanceCorrection();
        if (mDynamicCameraDistanceEnabled)
            mCameraDistance = std::min(mCameraDistance, mMaxNextCameraDistance);

        osg::Quat orient =  osg::Quat(getPitch(), osg::Vec3d(1,0,0)) * osg::Quat(getYaw(), osg::Vec3d(0,0,1));
        osg::Vec3d offset = orient * osg::Vec3d(0.f, -mCameraDistance, 0.f);
        MWPhysics::RayCastingResult result = rayCasting->castSphere(focal, focal + offset, cameraObstacleLimit, collisionType);
        if (result.mHit)
        {
            mCameraDistance = (result.mHitPos + result.mHitNormal * cameraObstacleLimit - focal).length();
            offset = orient * osg::Vec3d(0.f, -mCameraDistance, 0.f);
        }

        mPosition = focal + offset;
    }

    void Camera::setMode(Mode newMode, bool force)
    {
        if (newMode == Mode::StandingPreview)
            newMode = Mode::ThirdPerson;
        if (mMode == newMode)
            return;
        Mode oldMode = mMode;
        if (!force && (newMode == Mode::FirstPerson || oldMode == Mode::FirstPerson) && !mAnimation->upperBodyReady())
        {
            // Changing the view will stop all playing animations, so if we are playing
            // anything important, queue the view change for later
            mQueuedMode = newMode;
            return;
        }
        mMode = newMode;
        mQueuedMode = std::nullopt;
        if (newMode == Mode::FirstPerson)
            mFirstPersonView = true;
        else if (newMode == Mode::ThirdPerson)
            mFirstPersonView = false;
        calculateDeferredRotation();
        if (oldMode == Mode::FirstPerson || newMode == Mode::FirstPerson)
        {
            instantTransition();
            mProcessViewChange = true;
        }
        else if (newMode == Mode::ThirdPerson)
            updateStandingPreviewMode();
    }

    void Camera::updateStandingPreviewMode()
    {
        float speed = mTrackingPtr.getClass().getCurrentSpeed(mTrackingPtr);
        bool combat = mTrackingPtr.getClass().isActor() &&
                      mTrackingPtr.getClass().getCreatureStats(mTrackingPtr).getDrawState() != MWMechanics::DrawState_Nothing;
        bool standingStill = speed == 0 && !combat && mStandingPreviewAllowed;
        if (!standingStill && mMode == Mode::StandingPreview)
        {
            mMode = Mode::ThirdPerson;
            calculateDeferredRotation();
        }
        else if (standingStill && mMode == Mode::ThirdPerson)
            mMode = Mode::StandingPreview;
    }

    void Camera::setFocalPointTargetOffset(const osg::Vec2d& v)
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
        setMode(mFirstPersonView ? Mode::ThirdPerson : Mode::FirstPerson, force);
    }

    void Camera::allowVanityMode(bool allow)
    {
        if (!allow && mMode == Mode::Vanity)
            toggleVanityMode(false);
        mVanityAllowed = allow;
    }

    bool Camera::toggleVanityMode(bool enable)
    {
        if (!enable)
            setMode(mFirstPersonView ? Mode::FirstPerson : Mode::ThirdPerson, false);
        else if (mVanityAllowed)
            setMode(Mode::Vanity, false);
        return (mMode == Mode::Vanity) == enable;
    }

    void Camera::togglePreviewMode(bool enable)
    {
        if (mFirstPersonView && !mAnimation->upperBodyReady())
            return;
        if ((mMode == Mode::Preview) == enable)
            return;
        if (enable)
            setMode(Mode::Preview);
        else
            setMode(mFirstPersonView ? Mode::FirstPerson : Mode::ThirdPerson);
    }

    void Camera::setSneakOffset(float offset)
    {
        mAnimation->setFirstPersonOffset(osg::Vec3f(0,0,-offset));
    }

    void Camera::setYaw(float angle)
    {
        mYaw = Misc::normalizeAngle(angle);
    }

    void Camera::setPitch(float angle)
    {
        const float epsilon = 0.000001f;
        float limit = static_cast<float>(osg::PI_2) - epsilon;
        mPitch = std::clamp(angle, -limit, limit);
    }

    float Camera::getCameraDistance() const
    {
        return mMode == Mode::FirstPerson ? 0.f : mCameraDistance;
    }

    void Camera::adjustCameraDistance(float delta)
    {
        if (mMode == Mode::Static)
            return;
        if (mMode != Mode::FirstPerson)
        {
            if (mIsNearest && delta < 0.f && mMode != Mode::Preview && mMode != Mode::Vanity)
                toggleViewMode();
            else
                mBaseCameraDistance = std::min(mCameraDistance - getCameraDistanceCorrection(), mBaseCameraDistance) + delta;
        }
        else if (delta > 0.f)
        {
            toggleViewMode();
            mBaseCameraDistance = 0;
        }

        mIsNearest = mBaseCameraDistance <= mNearest;
        mBaseCameraDistance = std::clamp(mBaseCameraDistance, mNearest, mFurthest);
        Settings::Manager::setFloat("third person camera distance", "Camera", mBaseCameraDistance);
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

    void Camera::setAnimation(NpcAnimation *anim)
    {
        mAnimation = anim;
        mProcessViewChange = true;
    }

    void Camera::processViewChange()
    {
        if (mMode == Mode::FirstPerson)
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
        mProcessViewChange = false;
    }

    void Camera::applyDeferredPreviewRotationToPlayer(float dt)
    {
        if (mMode != Mode::ThirdPerson || mTrackingPtr.isEmpty())
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
        if (mMode == Mode::Static)
            return;
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
        if (mMode == Mode::Static)
        {
            mDeferredRotation = osg::Vec3f();
            return;
        }
        MWWorld::Ptr ptr = mTrackingPtr;
        if (mMode == Mode::Preview || mMode == Mode::Vanity || ptr.isEmpty())
            return;
        if (mFirstPersonView)
        {
            instantTransition();
            return;
        }

        mDeferredRotation.x() = Misc::normalizeAngle(-ptr.getRefData().getPosition().rot[0] - mPitch);
        mDeferredRotation.z() = Misc::normalizeAngle(-ptr.getRefData().getPosition().rot[2] - mYaw);
        if (!mDeferredRotationAllowed)
            mDeferredRotationDisabled = true;
    }

    bool Camera::isVanityOrPreviewModeEnabled() const
    {
        return mMode == Mode::Vanity || mMode == Mode::Preview || mMode == Mode::StandingPreview;
    }

}
