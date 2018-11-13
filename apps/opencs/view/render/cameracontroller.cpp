#include "cameracontroller.hpp"

#include <cmath>

#include <QWidget>

#include <osg/BoundingBox>
#include <osg/Camera>
#include <osg/ComputeBoundsVisitor>
#include <osg/Drawable>
#include <osg/Group>
#include <osg/Matrixd>
#include <osg/Quat>

#include <osgUtil/LineSegmentIntersector>

#include "../../model/prefs/shortcut.hpp"

#include "scenewidget.hpp"

namespace CSVRender
{

    /*
    Camera Controller
    */

    const osg::Vec3d CameraController::WorldUp = osg::Vec3d(0, 0, 1);

    const osg::Vec3d CameraController::LocalUp = osg::Vec3d(0, 1, 0);
    const osg::Vec3d CameraController::LocalLeft = osg::Vec3d(1, 0, 0);
    const osg::Vec3d CameraController::LocalForward = osg::Vec3d(0, 0, 1);

    CameraController::CameraController(QObject* parent)
        : QObject(parent)
        , mActive(false)
        , mInverted(false)
        , mCameraSensitivity(1/650.f)
        , mSecondaryMoveMult(50)
        , mWheelMoveMult(8)
        , mCamera(nullptr)
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
        bool wasActive = mActive;

        mCamera = camera;
        mActive = (mCamera != nullptr);

        if (mActive != wasActive)
        {
            for (std::vector<CSMPrefs::Shortcut*>::iterator it = mShortcuts.begin(); it != mShortcuts.end(); ++it)
            {
                CSMPrefs::Shortcut* shortcut = *it;
                shortcut->enable(mActive);
            }
        }
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

    void CameraController::addShortcut(CSMPrefs::Shortcut* shortcut)
    {
        mShortcuts.push_back(shortcut);
    }

    /*
    Free Camera Controller
    */

    FreeCameraController::FreeCameraController(QWidget* widget)
        : CameraController(widget)
        , mLockUpright(false)
        , mModified(false)
        , mNaviPrimary(false)
        , mNaviSecondary(false)
        , mFast(false)
        , mFastAlternate(false)
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
        CSMPrefs::Shortcut* naviPrimaryShortcut = new CSMPrefs::Shortcut("scene-navi-primary", widget);
        naviPrimaryShortcut->enable(false);
        connect(naviPrimaryShortcut, SIGNAL(activated(bool)), this, SLOT(naviPrimary(bool)));

        addShortcut(naviPrimaryShortcut);

        CSMPrefs::Shortcut* naviSecondaryShortcut = new CSMPrefs::Shortcut("scene-navi-secondary", widget);
        naviSecondaryShortcut->enable(false);
        connect(naviSecondaryShortcut, SIGNAL(activated(bool)), this, SLOT(naviSecondary(bool)));

        addShortcut(naviSecondaryShortcut);

        CSMPrefs::Shortcut* forwardShortcut = new CSMPrefs::Shortcut("free-forward", "scene-speed-modifier",
            CSMPrefs::Shortcut::SM_Detach, widget);
        forwardShortcut->enable(false);
        connect(forwardShortcut, SIGNAL(activated(bool)), this, SLOT(forward(bool)));
        connect(forwardShortcut, SIGNAL(secondary(bool)), this, SLOT(alternateFast(bool)));

        addShortcut(forwardShortcut);

        CSMPrefs::Shortcut* leftShortcut = new CSMPrefs::Shortcut("free-left", widget);
        leftShortcut->enable(false);
        connect(leftShortcut, SIGNAL(activated(bool)), this, SLOT(left(bool)));

        addShortcut(leftShortcut);

        CSMPrefs::Shortcut* backShortcut = new CSMPrefs::Shortcut("free-backward", widget);
        backShortcut->enable(false);
        connect(backShortcut, SIGNAL(activated(bool)), this, SLOT(backward(bool)));

        addShortcut(backShortcut);

        CSMPrefs::Shortcut* rightShortcut = new CSMPrefs::Shortcut("free-right", widget);
        rightShortcut->enable(false);
        connect(rightShortcut, SIGNAL(activated(bool)), this, SLOT(right(bool)));

        addShortcut(rightShortcut);

        CSMPrefs::Shortcut* rollLeftShortcut = new CSMPrefs::Shortcut("free-roll-left", widget);
        rollLeftShortcut->enable(false);
        connect(rollLeftShortcut, SIGNAL(activated(bool)), this, SLOT(rollLeft(bool)));

        addShortcut(rollLeftShortcut);

        CSMPrefs::Shortcut* rollRightShortcut = new CSMPrefs::Shortcut("free-roll-right", widget);
        rollRightShortcut->enable(false);
        connect(rollRightShortcut, SIGNAL(activated(bool)), this, SLOT(rollRight(bool)));

        addShortcut(rollRightShortcut);

        CSMPrefs::Shortcut* speedModeShortcut = new CSMPrefs::Shortcut("free-speed-mode", widget);
        speedModeShortcut->enable(false);
        connect(speedModeShortcut, SIGNAL(activated()), this, SLOT(swapSpeedMode()));

        addShortcut(speedModeShortcut);
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

    void FreeCameraController::handleMouseMoveEvent(int x, int y)
    {
        if (!isActive())
            return;

        if (mNaviPrimary)
        {
            double scalar = getCameraSensitivity() * (getInverted() ? -1.0 : 1.0);
            yaw(x * scalar);
            pitch(y * scalar);
        }
        else if (mNaviSecondary)
        {
            osg::Vec3d movement;
            movement += LocalLeft * -x * getSecondaryMovementMultiplier();
            movement += LocalUp * y * getSecondaryMovementMultiplier();

            translate(movement);
        }
    }

    void FreeCameraController::handleMouseScrollEvent(int x)
    {
        if (!isActive())
            return;

        translate(LocalForward * x * ((mFast ^ mFastAlternate) ? getWheelMovementMultiplier() : 1));
    }

    void FreeCameraController::update(double dt)
    {
        if (!isActive())
            return;

        double linDist = mLinSpeed * dt;
        double rotDist = mRotSpeed * dt;

        if (mFast ^ mFastAlternate)
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

    void FreeCameraController::naviPrimary(bool active)
    {
        mNaviPrimary = active;
    }

    void FreeCameraController::naviSecondary(bool active)
    {
        mNaviSecondary = active;
    }

    void FreeCameraController::forward(bool active)
    {
        mForward = active;
    }

    void FreeCameraController::left(bool active)
    {
        mLeft = active;
    }

    void FreeCameraController::backward(bool active)
    {
        mBackward = active;
    }

    void FreeCameraController::right(bool active)
    {
        mRight = active;
    }

    void FreeCameraController::rollLeft(bool active)
    {
        mRollLeft = active;
    }

    void FreeCameraController::rollRight(bool active)
    {
        mRollRight = active;
    }

    void FreeCameraController::alternateFast(bool active)
    {
        mFastAlternate = active;
    }

    void FreeCameraController::swapSpeedMode()
    {
        mFast = !mFast;
    }

    /*
    Orbit Camera Controller
    */

    OrbitCameraController::OrbitCameraController(QWidget* widget)
        : CameraController(widget)
        , mInitialized(false)
        , mNaviPrimary(false)
        , mNaviSecondary(false)
        , mFast(false)
        , mFastAlternate(false)
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
        , mConstRoll(false)
    {
        CSMPrefs::Shortcut* naviPrimaryShortcut = new CSMPrefs::Shortcut("scene-navi-primary", widget);
        naviPrimaryShortcut->enable(false);
        connect(naviPrimaryShortcut, SIGNAL(activated(bool)), this, SLOT(naviPrimary(bool)));

        addShortcut(naviPrimaryShortcut);

        CSMPrefs::Shortcut* naviSecondaryShortcut = new CSMPrefs::Shortcut("scene-navi-secondary", widget);
        naviSecondaryShortcut->enable(false);
        connect(naviSecondaryShortcut, SIGNAL(activated(bool)), this, SLOT(naviSecondary(bool)));

        addShortcut(naviSecondaryShortcut);

        CSMPrefs::Shortcut* upShortcut = new CSMPrefs::Shortcut("orbit-up", "scene-speed-modifier",
            CSMPrefs::Shortcut::SM_Detach, widget);
        upShortcut->enable(false);
        connect(upShortcut, SIGNAL(activated(bool)), this, SLOT(up(bool)));
        connect(upShortcut, SIGNAL(secondary(bool)), this, SLOT(alternateFast(bool)));

        addShortcut(upShortcut);

        CSMPrefs::Shortcut* leftShortcut = new CSMPrefs::Shortcut("orbit-left", widget);
        leftShortcut->enable(false);
        connect(leftShortcut, SIGNAL(activated(bool)), this, SLOT(left(bool)));

        addShortcut(leftShortcut);

        CSMPrefs::Shortcut* downShortcut = new CSMPrefs::Shortcut("orbit-down", widget);
        downShortcut->enable(false);
        connect(downShortcut, SIGNAL(activated(bool)), this, SLOT(down(bool)));

        addShortcut(downShortcut);

        CSMPrefs::Shortcut* rightShortcut = new CSMPrefs::Shortcut("orbit-right", widget);
        rightShortcut->enable(false);
        connect(rightShortcut, SIGNAL(activated(bool)), this, SLOT(right(bool)));

        addShortcut(rightShortcut);

        CSMPrefs::Shortcut* rollLeftShortcut = new CSMPrefs::Shortcut("orbit-roll-left", widget);
        rollLeftShortcut->enable(false);
        connect(rollLeftShortcut, SIGNAL(activated(bool)), this, SLOT(rollLeft(bool)));

        addShortcut(rollLeftShortcut);

        CSMPrefs::Shortcut* rollRightShortcut = new CSMPrefs::Shortcut("orbit-roll-right", widget);
        rollRightShortcut->enable(false);
        connect(rollRightShortcut, SIGNAL(activated(bool)), this, SLOT(rollRight(bool)));

        addShortcut(rollRightShortcut);

        CSMPrefs::Shortcut* speedModeShortcut = new CSMPrefs::Shortcut("orbit-speed-mode", widget);
        speedModeShortcut->enable(false);
        connect(speedModeShortcut, SIGNAL(activated()), this, SLOT(swapSpeedMode()));

        addShortcut(speedModeShortcut);
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

    void OrbitCameraController::handleMouseMoveEvent(int x, int y)
    {
        if (!isActive())
            return;

        if (!mInitialized)
            initialize();

        if (mNaviPrimary)
        {
            double scalar = getCameraSensitivity() * (getInverted() ? -1.0 : 1.0);
            rotateHorizontal(x * scalar);
            rotateVertical(-y * scalar);
        }
        else if (mNaviSecondary)
        {
            osg::Vec3d movement;
            movement += LocalLeft * x * getSecondaryMovementMultiplier();
            movement += LocalUp * -y * getSecondaryMovementMultiplier();

            translate(movement);
        }
    }

    void OrbitCameraController::handleMouseScrollEvent(int x)
    {
        if (!isActive())
            return;

        zoom(-x * ((mFast ^ mFastAlternate) ? getWheelMovementMultiplier() : 1));
    }

    void OrbitCameraController::update(double dt)
    {
        if (!isActive())
            return;

        if (!mInitialized)
            initialize();

        double rotDist = mOrbitSpeed * dt;

        if (mFast ^ mFastAlternate)
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

    void OrbitCameraController::reset()
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
    
    void OrbitCameraController::setConstRoll(bool enabled)
    {
        mConstRoll = enabled;
    }

    void OrbitCameraController::rotateHorizontal(double value)
    {
        osg::Vec3d eye, center, up;
        getCamera()->getViewMatrixAsLookAt(eye, center, up);
        osg::Vec3d absoluteUp = osg::Vec3(0,0,1);

        osg::Quat rotation = osg::Quat(value, mConstRoll ? absoluteUp : up);
        osg::Vec3d oldOffset = eye - mCenter;
        osg::Vec3d newOffset = rotation * oldOffset;

        if (mConstRoll)
            up = rotation * up;

        getCamera()->setViewMatrixAsLookAt(mCenter + newOffset, mCenter, up);
    }

    void OrbitCameraController::rotateVertical(double value)
    {
        osg::Vec3d eye, center, up;
        getCamera()->getViewMatrixAsLookAt(eye, center, up);

        osg::Vec3d forward = center - eye;
        osg::Vec3d axis = up ^ forward;

        osg::Quat rotation = osg::Quat(value,axis);
        osg::Vec3d oldOffset = eye - mCenter;
        osg::Vec3d newOffset = rotation * oldOffset;
            
        if (mConstRoll)
            up = rotation * up;

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

    void OrbitCameraController::naviPrimary(bool active)
    {
        mNaviPrimary = active;
    }

    void OrbitCameraController::naviSecondary(bool active)
    {
        mNaviSecondary = active;
    }

    void OrbitCameraController::up(bool active)
    {
        mUp = active;
    }

    void OrbitCameraController::left(bool active)
    {
        mLeft = active;
    }

    void OrbitCameraController::down(bool active)
    {
        mDown = active;
    }

    void OrbitCameraController::right(bool active)
    {
        mRight = active;
    }

    void OrbitCameraController::rollLeft(bool active)
    {
        if (isActive())
            mRollLeft = active;
    }

    void OrbitCameraController::rollRight(bool active)
    {
        mRollRight = active;
    }

    void OrbitCameraController::alternateFast(bool active)
    {
        mFastAlternate = active;
    }

    void OrbitCameraController::swapSpeedMode()
    {
        mFast = !mFast;
    }
}
