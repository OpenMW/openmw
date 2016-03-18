#include "cameracontroller.hpp"

#include <QKeyEvent>

#include <osg/Camera>
#include <osg/Matrixd>
#include <osg/Quat>

namespace CSVRender
{

    /*
    Camera Controller
    */

    const osg::Vec3d CameraController::WorldUp = osg::Vec3d(0, 0, 1);

    const osg::Vec3d CameraController::LocalUp = osg::Vec3d(0, 1, 0);
    const osg::Vec3d CameraController::LocalLeft = osg::Vec3d(1, 0, 0);
    const osg::Vec3d CameraController::LocalForward = osg::Vec3d(0, 0, 1);

    const double CameraController::LinearSpeed = 1000;
    const double CameraController::RotationalSpeed = osg::PI / 2.f;
    const double CameraController::SpeedMultiplier = 8;

    CameraController::CameraController()
        : mActive(false)
        , mModified(false)
        , mMouseScalar(-1/350.f)
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

    double CameraController::getMouseScalar() const
    {
        return mMouseScalar;
    }

    void CameraController::setCamera(osg::Camera* camera)
    {
        mCamera = camera;
        mActive = (mCamera != NULL);

        if (mActive)
            onActivate();
    }

    void CameraController::setMouseScalar(double value)
    {
        mMouseScalar = value;
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
    {
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
            yaw(x * getMouseScalar());
            pitch(y * getMouseScalar());
            setModified();
        }
        else if (mode == "s-navi")
        {
            translate(LocalLeft * x + LocalUp * -y);
            setModified();
        }
        else if (mode == "t-navi")
        {
            translate(LocalForward * x * (mFast ? SpeedMultiplier : 1));
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

        double linDist = LinearSpeed * dt;
        double rotDist = RotationalSpeed * dt;

        if (mFast)
            linDist *= SpeedMultiplier;

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
    {
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
            rotateHorizontal(x * getMouseScalar());
            rotateVertical(y * getMouseScalar());
            setModified();
        }
        else if (mode == "s-navi")
        {
            translate(LocalLeft * x + LocalUp * -y);
            setModified();
        }
        else if (mode == "t-navi")
        {
            zoom(x * (mFast ? SpeedMultiplier : 1));
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

        double rotDist = RotationalSpeed * dt;

        if (mFast)
            rotDist *= SpeedMultiplier;

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

        lookAtCenter();

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

        osg::Quat rotation = getCamera()->getViewMatrix().getRotate();
        osg::Vec3d position = getCamera()->getViewMatrix().getTrans();
        osg::Vec3d offset = rotation * (LocalForward * DefaultStartDistance);

        mCenter = position + offset;

        mInitialized = true;
    }

    void OrbitCameraController::rotateHorizontal(double value)
    {
        osg::Vec3d position = getCamera()->getViewMatrix().getTrans();
        osg::Vec3d offset = position - mCenter;
        osg::Quat rotation = getCamera()->getViewMatrix().getRotate();

        osg::Quat offsetRotation = osg::Quat(value, LocalUp);
        osg::Vec3d newOffset = (rotation * offsetRotation) * (rotation.inverse() * offset);

        getCamera()->getViewMatrix().setTrans(mCenter + newOffset);
    }

    void OrbitCameraController::rotateVertical(double value)
    {
        osg::Vec3d position = getCamera()->getViewMatrix().getTrans();
        osg::Vec3d offset = position - mCenter;
        osg::Quat rotation = getCamera()->getViewMatrix().getRotate();

        osg::Quat offsetRotation = osg::Quat(value, LocalLeft);
        osg::Vec3d newOffset = (rotation * offsetRotation) * (rotation.inverse() * offset);

        getCamera()->getViewMatrix().setTrans(mCenter + newOffset);
    }

    void OrbitCameraController::roll(double value)
    {
        getCamera()->getViewMatrix() *= osg::Matrixd::rotate(value, LocalForward);
    }

    void OrbitCameraController::translate(const osg::Vec3d& offset)
    {
        mCenter += offset;
        getCamera()->getViewMatrix() *= osg::Matrixd::translate(offset);
    }

    void OrbitCameraController::zoom(double value)
    {
        osg::Vec3d dir = mCenter - getCamera()->getViewMatrix().getTrans();
        double distance = dir.normalize();

        if (distance > 1 || value < 0)
        {
            getCamera()->getViewMatrix() *= osg::Matrixd::translate(dir * value);
        }
    }

    void OrbitCameraController::lookAtCenter()
    {
        osg::Vec3d position = getCamera()->getViewMatrix().getTrans();
        osg::Vec3d offset = mCenter - position;
        osg::Quat rotation = getCamera()->getViewMatrix().getRotate();

        osg::Quat newRotation;
        newRotation.makeRotate(LocalForward, offset);

        getCamera()->getViewMatrix().setRotate(newRotation);
    }
}
