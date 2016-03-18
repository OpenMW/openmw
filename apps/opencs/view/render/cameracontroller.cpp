#include "cameracontroller.hpp"

#include <QKeyEvent>

#include <osg/Camera>
#include <osg/Geode>
#include <osg/Group>
#include <osg/Matrixd>
#include <osg/Quat>
#include <osg/Shape>
#include <osg/ShapeDrawable>

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
        , mMouseScalar(-1/700.f)
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

    OrbitCameraController::OrbitCameraController(osg::Group* group)
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
        , mCenterNode(new osg::PositionAttitudeTransform())
    {
        group->addChild(mCenterNode);
        createShape();
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
            rotateVertical(-y * getMouseScalar());
            setModified();
        }
        else if (mode == "s-navi")
        {
            translate(LocalLeft * x + LocalUp * -y);
            setModified();
        }
        else if (mode == "t-navi")
        {
            zoom(-x * (mFast ? SpeedMultiplier : 1));
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

        mCenterNode->setPosition(mCenter);

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

        osg::Vec3d eye, up;
        getCamera()->getViewMatrixAsLookAt(eye, mCenter, up, DefaultStartDistance);

        mDistance = DefaultStartDistance;

        mInitialized = true;
    }

    void OrbitCameraController::createShape()
    {
        const float boxWidth = 100;

        osg::ref_ptr<osg::Box> box = new osg::Box(osg::Vec3f(0, 0, 0), boxWidth);
        osg::ref_ptr<osg::ShapeDrawable> drawable = new osg::ShapeDrawable(box);
        drawable->setColor(osg::Vec4f(0.f, 0.9f, 0.f, 1.f));

        osg::ref_ptr<osg::Geode> geode = new osg::Geode();
        geode->addChild(drawable);
        mCenterNode->addChild(geode);
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
        mCenter += offset;
        getCamera()->getViewMatrix() *= osg::Matrixd::translate(offset);
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
