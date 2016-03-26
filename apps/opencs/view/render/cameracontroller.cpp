#include "cameracontroller.hpp"

#include <QKeyEvent>

#include <osg/Camera>
#include <osg/Drawable>
#include <osg/Matrixd>
#include <osg/Quat>

#include <osgUtil/LineSegmentIntersector>

#include "mask.hpp"

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
        , mModified(false)
        , mCameraSensitivity(-1/700.f)
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

    bool CameraController::isModified() const
    {
        return mModified;
    }

    osg::Camera* CameraController::getCamera() const
    {
        return mCamera;
    }

    double CameraController::getCameraSensitivity() const
    {
        return mCameraSensitivity;
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

    void CameraController::setSecondaryMovementMultiplier(double value)
    {
        mSecondaryMoveMult = value;
    }

    void CameraController::setWheelMovementMultiplier(double value)
    {
        mWheelMoveMult = value;
    }

    void CameraController::setSceneBounds(const osg::BoundingBox& bounds, const osg::Vec3d& up)
    {
        osg::Vec3d eye = osg::Vec3d(bounds.xMax(), bounds.yMax(), bounds.zMax());
        osg::Vec3d center = bounds.center();

        getCamera()->setViewMatrixAsLookAt(eye, center, up);
    }

    void CameraController::setModified()
    {
        mModified = true;
    }

    void CameraController::resetModified()
    {
        mModified = false;
    }

    /*
    Free Camera Controller
    */

    FreeCameraController::FreeCameraController()
        : mLockUpright(false)
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
        setModified();
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
            setModified();
        }
        else if (event->key() == Qt::Key_E)
        {
            mRollRight = pressed;
            setModified();
        }
        else if (event->key() == Qt::Key_A)
        {
            mLeft = pressed;
            setModified();
        }
        else if (event->key() == Qt::Key_D)
        {
            mRight = pressed;
            setModified();
        }
        else if (event->key() == Qt::Key_W)
        {
            mForward = pressed;
            setModified();
        }
        else if (event->key() == Qt::Key_S)
        {
            mBackward = pressed;
            setModified();
        }
        else if (event->key() == Qt::Key_Shift)
        {
            mFast = pressed;
            setModified();
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
            yaw(x * getCameraSensitivity());
            pitch(y * getCameraSensitivity());
            setModified();
        }
        else if (mode == "s-navi")
        {
            osg::Vec3d movement;
            movement += LocalLeft * -x * getSecondaryMovementMultiplier();
            movement += LocalUp * y * getSecondaryMovementMultiplier();

            translate(movement);
            setModified();
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
        else if(isModified())
        {
            stabilize();
        }

        // Normalize the matrix to counter drift
        getCamera()->getViewMatrix().orthoNormal(getCamera()->getViewMatrix());

        resetModified();
    }

    void FreeCameraController::yaw(double value)
    {
        getCamera()->getViewMatrix() *= osg::Matrixd::rotate(value, LocalUp);
    }

    void FreeCameraController::pitch(double value)
    {
        getCamera()->getViewMatrix() *= osg::Matrixd::rotate(value, LocalLeft);
    }

    void FreeCameraController::roll(double value)
    {
        getCamera()->getViewMatrix() *= osg::Matrixd::rotate(value, LocalForward);
    }

    void FreeCameraController::translate(const osg::Vec3d& offset)
    {
        getCamera()->getViewMatrix() *= osg::Matrixd::translate(offset);
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
        , mCenter(0,0,0)
        , mDistance(0)
        , mOrbitSpeed(osg::PI / 4)
        , mOrbitSpeedMult(4)
    {
    }

    double OrbitCameraController::getOrbitSpeed() const
    {
        return mOrbitSpeed;
    }

    double OrbitCameraController::getOrbitSpeedMultiplier() const
    {
        return mOrbitSpeedMult;
    }

    void OrbitCameraController::setOrbitSpeed(double value)
    {
        mOrbitSpeed = value;
    }

    void OrbitCameraController::setOrbitSpeedMultiplier(double value)
    {
        mOrbitSpeedMult = value;
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
            setModified();
        }
        else if (event->key() == Qt::Key_E)
        {
            mRollRight = pressed;
            setModified();
        }
        else if (event->key() == Qt::Key_A)
        {
            mLeft = pressed;
            setModified();
        }
        else if (event->key() == Qt::Key_D)
        {
            mRight = pressed;
            setModified();
        }
        else if (event->key() == Qt::Key_W)
        {
            mUp = pressed;
            setModified();
        }
        else if (event->key() == Qt::Key_S)
        {
            mDown = pressed;
            setModified();
        }
        else if (event->key() == Qt::Key_Shift)
        {
            mFast = pressed;
            setModified();
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
            rotateHorizontal(x * getCameraSensitivity());
            rotateVertical(-y * getCameraSensitivity());
            setModified();
        }
        else if (mode == "s-navi")
        {
            osg::Vec3d movement;
            movement += LocalLeft * x * getSecondaryMovementMultiplier();
            movement += LocalUp * -y * getSecondaryMovementMultiplier();

            translate(movement);
            setModified();
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

        resetModified();
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

        visitor.setTraversalMask(Mask_Reference | Mask_Terrain);

        getCamera()->accept(visitor);

        osg::Vec3d eye, center, up;
        getCamera()->getViewMatrixAsLookAt(eye, center, up, DefaultStartDistance);

        if (intersector->getIntersections().begin() != intersector->getIntersections().end())
        {
            mCenter = intersector->getIntersections().begin()->getWorldIntersectPoint();
            mDistance = (eye - center).length();
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

    CameraComputeBoundsVisitor::CameraComputeBoundsVisitor(unsigned int mask)
        : mMask(mask)
    {
    }

    unsigned int CameraComputeBoundsVisitor::getMask() const
    {
        return mMask;
    }

    void CameraComputeBoundsVisitor::setMask(unsigned int value)
    {
        mMask = value;
    }

    void CameraComputeBoundsVisitor::apply(osg::Drawable& drawable)
    {
        if (drawable.getNodeMask() & mMask)
            ComputeBoundsVisitor::apply(drawable);
    }

    void CameraComputeBoundsVisitor::apply(osg::Transform& transform)
    {
        if (transform.getNodeMask() & mMask)
            ComputeBoundsVisitor::apply(transform);
    }
}
