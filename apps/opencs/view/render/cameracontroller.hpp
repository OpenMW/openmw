#ifndef OPENCS_VIEW_CAMERACONTROLLER_H
#define OPENCS_VIEW_CAMERACONTROLLER_H

#include <string>
#include <vector>

#include <QObject>

#include <osg/ref_ptr>
#include <osg/Vec3d>

namespace osg
{
    class Camera;
    class Group;
}

namespace CSMPrefs
{
    class Shortcut;
}

namespace CSVRender
{
    class SceneWidget;

    class CameraController : public QObject
    {
            Q_OBJECT

        public:

            static const osg::Vec3d WorldUp;

            static const osg::Vec3d LocalUp;
            static const osg::Vec3d LocalLeft;
            static const osg::Vec3d LocalForward;

            CameraController(QObject* parent);
            virtual ~CameraController();

            bool isActive() const;

            osg::Camera* getCamera() const;
            double getCameraSensitivity() const;
            bool getInverted() const;
            double getSecondaryMovementMultiplier() const;
            double getWheelMovementMultiplier() const;

            void setCamera(osg::Camera*);
            void setCameraSensitivity(double value);
            void setInverted(bool value);
            void setSecondaryMovementMultiplier(double value);
            void setWheelMovementMultiplier(double value);

            // moves the camera to an intelligent position
            void setup(osg::Group* root, unsigned int mask, const osg::Vec3d& up);

            virtual void handleMouseMoveEvent(int x, int y) = 0;
            virtual void handleMouseScrollEvent(int x) = 0;

            virtual void update(double dt) = 0;

        protected:

            void addShortcut(CSMPrefs::Shortcut* shortcut);

        private:

            bool mActive, mInverted;
            double mCameraSensitivity;
            double mSecondaryMoveMult;
            double mWheelMoveMult;

            osg::Camera* mCamera;

            std::vector<CSMPrefs::Shortcut*> mShortcuts;
    };

    class FreeCameraController : public CameraController
    {
            Q_OBJECT

        public:

            FreeCameraController(QWidget* parent);

            double getLinearSpeed() const;
            double getRotationalSpeed() const;
            double getSpeedMultiplier() const;

            void setLinearSpeed(double value);
            void setRotationalSpeed(double value);
            void setSpeedMultiplier(double value);

            void fixUpAxis(const osg::Vec3d& up);
            void unfixUpAxis();

            void handleMouseMoveEvent(int x, int y) override;
            void handleMouseScrollEvent(int x) override;

            void update(double dt) override;

        private:

            void yaw(double value);
            void pitch(double value);
            void roll(double value);
            void translate(const osg::Vec3d& offset);

            void stabilize();

            bool mLockUpright, mModified;
            bool mNaviPrimary, mNaviSecondary;
            bool mFast, mFastAlternate;
            bool mLeft, mRight, mForward, mBackward, mRollLeft, mRollRight;
            osg::Vec3d mUp;

            double mLinSpeed;
            double mRotSpeed;
            double mSpeedMult;

        private slots:

            void naviPrimary(bool active);
            void naviSecondary(bool active);
            void forward(bool active);
            void left(bool active);
            void backward(bool active);
            void right(bool active);
            void rollLeft(bool active);
            void rollRight(bool active);
            void alternateFast(bool active);
            void swapSpeedMode();
    };

    class OrbitCameraController : public CameraController
    {
            Q_OBJECT

        public:

            OrbitCameraController(QWidget* parent);

            osg::Vec3d getCenter() const;
            double getOrbitSpeed() const;
            double getOrbitSpeedMultiplier() const;
            unsigned int getPickingMask() const;

            void setCenter(const osg::Vec3d& center);
            void setOrbitSpeed(double value);
            void setOrbitSpeedMultiplier(double value);
            void setPickingMask(unsigned int value);

            void handleMouseMoveEvent(int x, int y) override;
            void handleMouseScrollEvent(int x) override;

            void update(double dt) override;

            /// \brief Flag controller to be re-initialized.
            void reset();

            void setConstRoll(bool enable);

        private:

            void initialize();

            void rotateHorizontal(double value);
            void rotateVertical(double value);
            void roll(double value);
            void translate(const osg::Vec3d& offset);
            void zoom(double value);

            bool mInitialized;
            bool mNaviPrimary, mNaviSecondary;
            bool mFast, mFastAlternate;
            bool mLeft, mRight, mUp, mDown, mRollLeft, mRollRight;
            unsigned int mPickingMask;
            osg::Vec3d mCenter;
            double mDistance;

            double mOrbitSpeed;
            double mOrbitSpeedMult;

            bool mConstRoll;

        private slots:

            void naviPrimary(bool active);
            void naviSecondary(bool active);
            void up(bool active);
            void left(bool active);
            void down(bool active);
            void right(bool active);
            void rollLeft(bool active);
            void rollRight(bool active);
            void alternateFast(bool active);
            void swapSpeedMode();
    };
}

#endif
