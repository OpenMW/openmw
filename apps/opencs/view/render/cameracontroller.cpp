#include "cameracontroller.hpp"

#include <cmath>

#include <QKeyEvent>

#include <osg/BoundingBox>
#include <osg/Camera>
#include <osg/ComputeBoundsVisitor>
#include <osg/Drawable>
#include <osg/Group>
#include <osg/Matrixd>
#include <osg/Quat>

#include <osgUtil/LineSegmentIntersector>

namespace CSVRender
{

    /*
    Camera Controller
    */

    const osg::Vec3d CameraController::WorldUp = osg::Vec3d(0, 0, 1);

    const osg::Vec3d CameraController::LocalUp = osg::Vec3d(0, 1, 0);
    const osg::Vec3d CameraController::LocalLeft = osg::Vec3d(1, 0, 0);
    const osg::Vec3d CameraController::LocalForward = osg::Vec3d(0, 0, 1);

    CameraController::CameraController()
        : mActive(false)
        , mInverted(false)
        , mCameraSensitivity(1/650.f)
        , mSecondaryMoveMult(50)
        , mWheelMoveMult(8)
        , mCamera(NULL)
    {
    }

    CameraController::~CameraController()
    {
    }

    bool CameraController::isActive() const
    {
        return mActive;
    }

    osg::Camera* CameraController::getCamera() const
    {
        return mCamera;
    }

    double CameraController::getCameraSensitivity() const
    {
        return mCameraSensitivity;
    }

    bool CameraController::getInverted() const
    {
        return mInverted;
    }

    double CameraController::getSecondaryMovementMultiplier() const
    {
        return mSecondaryMoveMult;
    }

    double CameraController::getWheelMovementMultiplier() const
    {
        return mWheelMoveMult;
    }

    void CameraController::setCamera(osg::Camera* camera)
    {
        mCamera = camera;
        mActive = (mCamera != NULL);

        if (mActive)
            onActivate();
    }

    void CameraController::setCameraSensitivity(double value)
    {
        mCameraSensitivity = value;
    }

    void CameraController::setInverted(bool value)
    {
        mInverted = value;
    }

    void CameraController::setSecondaryMovementMultiplier(double value)
    {
        mSecondaryMoveMult = value;
    }

    void CameraController::setWheelMovementMultiplier(double value)
    {
        mWheelMoveMult = value;
    }

    void CameraController::setup(osg::Group* root, unsigned int mask, const osg::Vec3d& up)
    {
        // Find World bounds
        osg::ComputeBoundsVisitor boundsVisitor;
        osg::BoundingBox& boundingBox = boundsVisitor.getBoundingBox();

        boundsVisitor.setTraversalMask(mask);
        root->accept(boundsVisitor);

        if (!boundingBox.valid())
        {
            // Try again without any mask
            boundsVisitor.reset();
            boundsVisitor.setTraversalMask(~0);
            root->accept(boundsVisitor);

            // Last resort, set a default
            if (!boundingBox.valid())
            {
                boundingBox.set(-1, -1, -1, 1, 1, 1);
            }
        }

        // Calculate a good starting position
        osg::Vec3d minBounds = boundingBox.corner(0) - boundingBox.center();
        osg::Vec3d maxBounds = boundingBox.corner(7) - boundingBox.center();

        osg::Vec3d camOffset = up * maxBounds > 0 ? maxBounds : minBounds;
        camOffset *= 2;

        osg::Vec3d eye = camOffset + boundingBox.center();
        osg::Vec3d center = boundingBox.center();

        getCamera()->setViewMatrixAsLookAt(eye, center, up);
    }

    /*
    Free Camera Controller
    */

    FreeCameraController::FreeCameraController()
        : mLockUpright(false)
        , mModified(false)
        , mFast(false)
        , mLeft(false)
        , mRight(false)
        , mForward(false)
        , mBackward(false)
        , mRollLeft(false)
        , mRollRight(false)
        , mUp(LocalUp)
        , mLinSpeed(1000)
        , mRotSpeed(osg::PI / 2)
        , mSpeedMult(8)
    {
    }

    double FreeCameraController::getLinearSpeed() const
    {
        return mLinSpeed;
    }

    double FreeCameraController::getRotationalSpeed() const
    {
        return mRotSpeed;
    }

    double FreeCameraController::getSpeedMultiplier() const
    {
        return mSpeedMult;
    }

    void FreeCameraController::setLinearSpeed(double value)
    {
        mLinSpeed = value;
    }

    void FreeCameraController::setRotationalSpeed(double value)
    {
        mRotSpeed = value;
    }

    void FreeCameraController::setSpeedMultiplier(double value)
    {
        mSpeedMult = value;
    }

    void FreeCameraController::fixUpAxis(const osg::Vec3d& up)
    {
        mLockUpright = true;
        mUp = up;
        mModified = true;
    }

    void FreeCameraController::unfixUpAxis()
    {
        mLockUpright = false;
    }

    bool FreeCameraController::handleKeyEvent(QKeyEvent* event, bool pressed)
    {
        if (!isActive())
            return false;

        if (event->key() == Qt::Key_Q)
        {
            mRollLeft = pressed;
        }
        else if (event->key() == Qt::Key_E)
        {
            mRollRight = pressed;
        }
        else if (event->key() == Qt::Key_A)
        {
            mLeft = pressed;
        }
        else if (event->key() == Qt::Key_D)
        {
            mRight = pressed;
        }
        else if (event->key() == Qt::Key_W)
        {
            mForward = pressed;
        }
        else if (event->key() == Qt::Key_S)
        {
            mBackward = pressed;
        }
        else if (event->key() == Qt::Key_Shift)
        {
            mFast = pressed;
        }
        else
        {
            return false;
        }

        return true;
    }

    bool FreeCameraController::handleMouseMoveEvent(std::string mode, int x, int y)
    {
        if (!isActive())
            return false;

        if (mode == "p-navi")
        {
            double scalar = getCameraSensitivity() * (getInverted() ? -1.0 : 1.0);
            yaw(x * scalar);
            pitch(y * scalar);
        }
        else if (mode == "s-navi")
        {
            osg::Vec3d movement;
            movement += LocalLeft * -x * getSecondaryMovementMultiplier();
            movement += LocalUp * y * getSecondaryMovementMultiplier();

            translate(movement);
        }
        else if (mode == "t-navi")
        {
            translate(LocalForward * x * (mFast ? getWheelMovementMultiplier() : 1));
        }
        else
        {
            return false;
        }

        return true;
    }

    void FreeCameraController::update(double dt)
    {
        if (!isActive())
            return;

        double linDist = mLinSpeed * dt;
        double rotDist = mRotSpeed * dt;

        if (mFast)
            linDist *= mSpeedMult;

        if (mLeft)
            translate(LocalLeft * linDist);
        if (mRight)
            translate(LocalLeft * -linDist);
        if (mForward)
            translate(LocalForward * linDist);
        if (mBackward)
            translate(LocalForward * -linDist);

        if (!mLockUpright)
        {
            if (mRollLeft)
                roll(-rotDist);
            if (mRollRight)
                roll(rotDist);
        }
        else if(mModified)
        {
            stabilize();
            mModified = false;
        }

        // Normalize the matrix to counter drift
        getCamera()->getViewMatrix().orthoNormal(getCamera()->getViewMatrix());
    }

    void FreeCameraController::resetInput()
    {
        mFast = false;
        mLeft = false;
        mRight = false;
        mForward = false;
        mBackward = false;
        mRollLeft = false;
        mRollRight = false;
    }

    void FreeCameraController::yaw(double value)
    {
        getCamera()->getViewMatrix() *= osg::Matrixd::rotate(value, LocalUp);
        mModified = true;
    }

    void FreeCameraController::pitch(double value)
    {
        const double Constraint = osg::PI / 2 - 0.1;

        if (mLockUpright)
        {
            osg::Vec3d eye, center, up;
            getCamera()->getViewMatrixAsLookAt(eye, center, up);

            osg::Vec3d forward = center - eye;
            osg::Vec3d left = up ^ forward;

            double pitchAngle = std::acos(up * mUp);
            if ((mUp ^ up) * left < 0)
                pitchAngle *= -1;

            if (std::abs(pitchAngle + value) > Constraint)
                value = (pitchAngle > 0 ? 1 : -1) * Constraint - pitchAngle;
        }

        getCamera()->getViewMatrix() *= osg::Matrixd::rotate(value, LocalLeft);
        mModified = true;
    }

    void FreeCameraController::roll(double value)
    {
        getCamera()->getViewMatrix() *= osg::Matrixd::rotate(value, LocalForward);
        mModified = true;
    }

    void FreeCameraController::translate(const osg::Vec3d& offset)
    {
        getCamera()->getViewMatrix() *= osg::Matrixd::translate(offset);
        mModified = true;
    }

    void FreeCameraController::stabilize()
    {
        osg::Vec3d eye, center, up;
        getCamera()->getViewMatrixAsLookAt(eye, center, up);
        getCamera()->setViewMatrixAsLookAt(eye, center, mUp);
    }

    /*
    Orbit Camera Controller
    */

    OrbitCameraController::OrbitCameraController()
        : mInitialized(false)
        , mFast(false)
        , mLeft(false)
        , mRight(false)
        , mUp(false)
        , mDown(false)
        , mRollLeft(false)
        , mRollRight(false)
        , mPickingMask(~0)
        , mCenter(0,0,0)
        , mDistance(0)
        , mOrbitSpeed(osg::PI / 4)
        , mOrbitSpeedMult(4)
    {
    }

    osg::Vec3d OrbitCameraController::getCenter() const
    {
        return mCenter;
    }

    double OrbitCameraController::getOrbitSpeed() const
    {
        return mOrbitSpeed;
    }

    double OrbitCameraController::getOrbitSpeedMultiplier() const
    {
        return mOrbitSpeedMult;
    }

    unsigned int OrbitCameraController::getPickingMask() const
    {
        return mPickingMask;
    }

    void OrbitCameraController::setCenter(const osg::Vec3d& value)
    {
        osg::Vec3d eye, center, up;
        getCamera()->getViewMatrixAsLookAt(eye, center, up);

        mCenter = value;
        mDistance = (eye - mCenter).length();

        getCamera()->setViewMatrixAsLookAt(eye, mCenter, up);

        mInitialized = true;
    }

    void OrbitCameraController::setOrbitSpeed(double value)
    {
        mOrbitSpeed = value;
    }

    void OrbitCameraController::setOrbitSpeedMultiplier(double value)
    {
        mOrbitSpeedMult = value;
    }

    void OrbitCameraController::setPickingMask(unsigned int value)
    {
        mPickingMask = value;
    }

    bool OrbitCameraController::handleKeyEvent(QKeyEvent* event, bool pressed)
    {
        if (!isActive())
            return false;

        if (!mInitialized)
            initialize();

        if (event->key() == Qt::Key_Q)
        {
            mRollLeft = pressed;
        }
        else if (event->key() == Qt::Key_E)
        {
            mRollRight = pressed;
        }
        else if (event->key() == Qt::Key_A)
        {
            mLeft = pressed;
        }
        else if (event->key() == Qt::Key_D)
        {
            mRight = pressed;
        }
        else if (event->key() == Qt::Key_W)
        {
            mUp = pressed;
        }
        else if (event->key() == Qt::Key_S)
        {
            mDown = pressed;
        }
        else if (event->key() == Qt::Key_Shift)
        {
            mFast = pressed;
        }
        else
        {
            return false;
        }

        return true;
    }

    bool OrbitCameraController::handleMouseMoveEvent(std::string mode, int x, int y)
    {
        if (!isActive())
            return false;

        if (!mInitialized)
            initialize();

        if (mode == "p-navi")
        {
            double scalar = getCameraSensitivity() * (getInverted() ? -1.0 : 1.0);
            rotateHorizontal(x * scalar);
            rotateVertical(-y * scalar);
        }
        else if (mode == "s-navi")
        {
            osg::Vec3d movement;
            movement += LocalLeft * x * getSecondaryMovementMultiplier();
            movement += LocalUp * -y * getSecondaryMovementMultiplier();

            translate(movement);
        }
        else if (mode == "t-navi")
        {
            zoom(-x * (mFast ? getWheelMovementMultiplier() : 1));
        }
        else
        {
            return false;
        }

        return true;
    }

    void OrbitCameraController::update(double dt)
    {
        if (!isActive())
            return;

        if (!mInitialized)
            initialize();

        double rotDist = mOrbitSpeed * dt;

        if (mFast)
            rotDist *= mOrbitSpeedMult;

        if (mLeft)
            rotateHorizontal(-rotDist);
        if (mRight)
            rotateHorizontal(rotDist);
        if (mUp)
            rotateVertical(rotDist);
        if (mDown)
            rotateVertical(-rotDist);

        if (mRollLeft)
            roll(-rotDist);
        if (mRollRight)
            roll(rotDist);

        // Normalize the matrix to counter drift
        getCamera()->getViewMatrix().orthoNormal(getCamera()->getViewMatrix());
    }

    void OrbitCameraController::resetInput()
    {
        mFast = false;
        mLeft = false;
        mRight =false;
        mUp = false;
        mDown = false;
        mRollLeft = false;
        mRollRight = false;
    }

    void OrbitCameraController::onActivate()
    {
        mInitialized = false;
    }

    void OrbitCameraController::initialize()
    {
        static const int DefaultStartDistance = 10000.f;

        // Try to intelligently pick focus object
        osg::ref_ptr<osgUtil::LineSegmentIntersector> intersector (new osgUtil::LineSegmentIntersector(
            osgUtil::Intersector::PROJECTION, osg::Vec3d(0, 0, 0), LocalForward));

        intersector->setIntersectionLimit(osgUtil::LineSegmentIntersector::LIMIT_NEAREST);
        osgUtil::IntersectionVisitor visitor(intersector);

        visitor.setTraversalMask(mPickingMask);

        getCamera()->accept(visitor);

        osg::Vec3d eye, center, up;
        getCamera()->getViewMatrixAsLookAt(eye, center, up, DefaultStartDistance);

        if (intersector->getIntersections().begin() != intersector->getIntersections().end())
        {
            mCenter = intersector->getIntersections().begin()->getWorldIntersectPoint();
            mDistance = (eye - mCenter).length();
        }
        else
        {
            mCenter = center;
            mDistance = DefaultStartDistance;
        }

        mInitialized = true;
    }

    void OrbitCameraController::rotateHorizontal(double value)
    {
        osg::Vec3d eye, center, up;
        getCamera()->getViewMatrixAsLookAt(eye, center, up);

        osg::Quat rotation = osg::Quat(value, up);
        osg::Vec3d oldOffset = eye - mCenter;
        osg::Vec3d newOffset = rotation * oldOffset;

        getCamera()->setViewMatrixAsLookAt(mCenter + newOffset, mCenter, up);
    }

    void OrbitCameraController::rotateVertical(double value)
    {
        osg::Vec3d eye, center, up;
        getCamera()->getViewMatrixAsLookAt(eye, center, up);

        osg::Vec3d forward = center - eye;
        osg::Quat rotation = osg::Quat(value, up ^ forward);
        osg::Vec3d oldOffset = eye - mCenter;
        osg::Vec3d newOffset = rotation * oldOffset;

        getCamera()->setViewMatrixAsLookAt(mCenter + newOffset, mCenter, up);
    }

    void OrbitCameraController::roll(double value)
    {
        getCamera()->getViewMatrix() *= osg::Matrixd::rotate(value, LocalForward);
    }

    void OrbitCameraController::translate(const osg::Vec3d& offset)
    {
        osg::Vec3d eye, center, up;
        getCamera()->getViewMatrixAsLookAt(eye, center, up);

        osg::Vec3d newOffset = getCamera()->getViewMatrix().getRotate().inverse() * offset;
        mCenter += newOffset;
        eye += newOffset;

        getCamera()->setViewMatrixAsLookAt(eye, mCenter, up);
    }

    void OrbitCameraController::zoom(double value)
    {
        mDistance = std::max(10., mDistance + value);

        osg::Vec3d eye, center, up;
        getCamera()->getViewMatrixAsLookAt(eye, center, up, 1.f);

        osg::Vec3d offset = (eye - center) * mDistance;

        getCamera()->setViewMatrixAsLookAt(mCenter + offset, mCenter, up);
    }
}
