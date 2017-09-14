#include "camera.hpp"

#include <osg/Camera>

#include <components/sceneutil/positionattitudetransform.hpp>
#include <osg/PositionAttitudeTransform>
#include <components/settings/settings.hpp>
#include "../mwbase/environment.hpp"
#include "../mwbase/windowmanager.hpp"

#include "../mwworld/ptr.hpp"
#include "../mwworld/refdata.hpp"

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

    const float Camera::sResetThirdPersonCameraDistance = 192.f;

    Camera::Camera (osg::Camera* camera)
    : mHeightScale(1.f),
      mCamera(camera),
      mAnimation(NULL),
      mCameraView(CameraView_FirstPerson),
      mActiveThirdPersonView(CameraView_ThirdPersonCenter),
      mPreviewMode(false),
      mFreeLook(true),
      mNearest(30.f),
      mFurthest(800.f),
      mIsNearest(false),
      mHeight(124.f),
      mMaxCameraDistance(Settings::Manager::getBool("third person over shoulder", "Input") ? Settings::Manager::getFloat("third person over shoulder camera distance", "Input") : sResetThirdPersonCameraDistance),
      mVanityToggleQueued(false),
      mVanityToggleQueuedValue(false),
      mViewModeToggleQueued(false),
      mCameraDistance(0.f),
      mCameraXOffsetFromCenter(0.0f),
      mCameraZOffsetFromCenter(0.0f),
      mCameraRotationDisjoint(false)
    {
        mVanity.enabled = false;
        mVanity.allowed = true;

        mPreviewCam.pitch = 0.f;
        mPreviewCam.yaw = 0.f;
        mPreviewCam.offset = 300.f;//400.f;
        mMainCam.pitch = 0.f;
        mMainCam.yaw = 0.f;
        mMainCam.offset = 300.f;//400.f;

        mCameraDistance = mMaxCameraDistance;

        mUpdateCallback = new UpdateRenderCameraCallback(this);
        mCamera->addUpdateCallback(mUpdateCallback);
        //mCamera->asPositionAttitudeTransform()->getAttitude();
    }

    Camera::~Camera()
    {
        mCamera->removeUpdateCallback(mUpdateCallback);
    }

    MWWorld::Ptr Camera::getTrackingPtr() const
    {
        return mTrackingPtr;
    }

    osg::Vec3d Camera::getFocalPoint()
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
            position.z() += mHeight * mHeightScale;
        return position;
    }

    void Camera::updateCamera(osg::Camera *cam)
    {
        if (mTrackingPtr.isEmpty())
            return;

        osg::Vec3d position = getFocalPoint();

        osg::Quat orient =  osg::Quat(getPitch(), osg::Vec3d(1,0,0)) * osg::Quat(getYaw(), osg::Vec3d(0,0,1));

        osg::Vec3d offset = orient * osg::Vec3d(mCameraXOffsetFromCenter, isFirstPerson() ? 0 : -mCameraDistance, mCameraZOffsetFromCenter);
        position += offset;

        osg::Vec3d forward = orient * osg::Vec3d(0,1,0);
        osg::Vec3d up = orient * osg::Vec3d(0,0,1);

        cam->setViewMatrixAsLookAt(position, position + forward, up);
    }

    void Camera::reset()
    {
        togglePreviewMode(false);
        toggleVanityMode(false);
        if (mCameraView != CameraView_FirstPerson)
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

        // only show the crosshair in game mode and in first person mode.
        MWBase::WindowManager *wm = MWBase::Environment::get().getWindowManager();
        wm->showCrosshair(
            !wm->isGuiMode() && 
            (
                (mCameraView & CameraView_ShowCrosshair) &&
                !mVanity.enabled && !mPreviewMode
            )
        );

        if(mVanity.enabled)
        {
            rotateCamera(0.f, osg::DegreesToRadians(3.f * duration), true);
        }
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

        // Cycle between our active Third Person View and First Person
        mActiveThirdPersonView = Settings::Manager::getBool("third person over shoulder", "Input") ? CameraView_ThirdPersonOverShoulder : CameraView_ThirdPersonCenter;
        changeToViewMode((mCameraView == CameraView_FirstPerson) ? mActiveThirdPersonView : CameraView_FirstPerson);
       
    }

    void Camera::changeToViewMode(CameraView newCameraView)
    {
        mCameraView = newCameraView;
        // On toggle set the max distance based on the type of third person camera view.  
        if ((mCameraView & CameraView_AnyThirdPersonNonCombat) && !Settings::Manager::getBool("allow third person zoom", "Input"))
            mMaxCameraDistance = (mCameraView == CameraView_ThirdPersonOverShoulder) ? Settings::Manager::getFloat("third person over shoulder camera distance", "Input") : sResetThirdPersonCameraDistance;

        processViewChange();
    }
    
    void Camera::toggleThirdPersonOverShouldRangedCamera() {
        changeToViewMode(
            (mCameraView == CameraView_ThirdPersonOverTheShoulderRanged) ? mActiveThirdPersonView : CameraView_ThirdPersonOverTheShoulderRanged
        );
    }

    void Camera::setThirdPersonOverShouldRangedCamera(bool set) {
        changeToViewMode(
            !set ? mActiveThirdPersonView : CameraView_ThirdPersonOverTheShoulderRanged
        );
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
        if ((mCameraView == CameraView_FirstPerson) && !mAnimation->upperBodyReady())
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
        if ((mCameraView == CameraView_FirstPerson) && !mAnimation->upperBodyReady())
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

    float Camera::getYaw()
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

    float Camera::getPitch()
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

    void Camera::setCameraDistance(float dist, bool adjust, bool override)
    {
        if((mCameraView == CameraView_FirstPerson) && !mPreviewMode && !mVanity.enabled)
            return;

        mIsNearest = false;

        if (adjust)
            dist += mCameraDistance;

        if (dist >= mFurthest) {
            dist = mFurthest;
        } else if (!override && dist < 10.f) {
            dist = 10.f;
        } else if (override && dist <= mNearest) {
            dist = mNearest;
            mIsNearest = true;
        }
        mCameraDistance = dist;

        if (override) {
            if (mVanity.enabled || mPreviewMode) {
                mPreviewCam.offset = mCameraDistance;
            } else if (mCameraView != CameraView_FirstPerson) {
                mMaxCameraDistance = mCameraDistance;
            }
        }
    }

    void Camera::setCameraDistance()
    {
        if (mVanity.enabled || mPreviewMode) {
            mCameraDistance = mPreviewCam.offset;
        }
        else if (mCameraView != CameraView_FirstPerson) {
            if (mCameraView == CameraView_ThirdPersonCenter){
                mCameraDistance = mMaxCameraDistance;
            }
            else if (mCameraView == CameraView_ThirdPersonOverShoulder) {
                mCameraDistance = mMaxCameraDistance;
            }
            else if (mCameraView == CameraView_ThirdPersonOverTheShoulderRanged) {
                mCameraDistance = 60.0f;
            }
        }
    }

    void Camera::setAnimation(NpcAnimation *anim)
    {
        mAnimation = anim;

        processViewChange();
    }

    void Camera::processViewChange()
    {
        // Except for over the shoulder, all view changes want to be centered on x and z
        mCameraXOffsetFromCenter = mCameraZOffsetFromCenter = 0.0f;
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
            // Set animation to normal for any third person option
            mAnimation->setViewMode(NpcAnimation::VM_Normal);
            SceneUtil::PositionAttitudeTransform* transform = mTrackingPtr.getRefData().getBaseNode();
            mTrackingNode = transform;
            if (transform)
                mHeightScale = transform->getScale().z();
            else
                mHeightScale = 1.f;
            if (!mPreviewMode && !mVanity.enabled) {
                if (mCameraView == CameraView_ThirdPersonOverShoulder) {
                    mCameraZOffsetFromCenter = Settings::Manager::getFloat("third person over shoulder z offset", "Input");
                    mCameraXOffsetFromCenter = Settings::Manager::getFloat("third person over shoulder x offset", "Input");
                }
                else if(mCameraView == CameraView_ThirdPersonOverTheShoulderRanged){
                    mCameraZOffsetFromCenter = 0.0f;
                    mCameraXOffsetFromCenter = 40.0f;
                }
            }
        }
        rotateCamera(getPitch(), getYaw(), false);
    }

    void Camera::getPosition(osg::Vec3f &focal, osg::Vec3f &camera)
    {
        focal = getFocalPoint();

        osg::Vec3d offset = getOrientation() * osg::Vec3d(mCameraXOffsetFromCenter, isFirstPerson() ? 0 : -mCameraDistance, mCameraZOffsetFromCenter);
        camera = focal + offset;
    }

    void Camera::togglePlayerLooking(bool enable)
    {
        mFreeLook = enable;
    }

    bool Camera::isVanityOrPreviewModeEnabled()
    {
        return mPreviewMode || mVanity.enabled;
    }

    bool Camera::isNearest()
    {
        return mIsNearest;
    }
}
