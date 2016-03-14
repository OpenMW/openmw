#ifndef OPENCS_VIEW_CAMERACONTROLLER_H
#define OPENCS_VIEW_CAMERACONTROLLER_H

#include <string>

#include <osg/ref_ptr>
#include <osg/Vec3d>

class QKeyEvent;

namespace osg
{
    class Camera;
}

namespace CSVRender
{
    class CameraController
    {
        public:

            static const osg::Vec3d LocalUp;
            static const osg::Vec3d LocalLeft;
            static const osg::Vec3d LocalForward;

            static const double LinearSpeed;
            static const double RotationalSpeed;
            static const double SpeedMultiplier;

            CameraController();
            virtual ~CameraController();

            bool isActive() const;
            bool isModified() const;

            osg::Camera* getCamera() const;
            double getMouseScalar() const;

            void setCamera(osg::Camera*);
            void setMouseScalar(double value);

            virtual bool handleKeyEvent(QKeyEvent* event, bool pressed) = 0;
            virtual bool handleMouseMoveEvent(std::string mode, int x, int y) = 0;

            virtual void update(double dt) = 0;

        protected:

            void setModified();
            void resetModified();

            virtual void onActivate(){}

        private:

            bool mActive, mModified;
            double mMouseScalar;

            osg::Camera* mCamera;
    };

    class FreeCameraController : public CameraController
    {
        public:

            FreeCameraController();

            void fixUpAxis(const osg::Vec3d& up);
            void unfixUpAxis();

            bool handleKeyEvent(QKeyEvent* event, bool pressed);
            bool handleMouseMoveEvent(std::string mode, int x, int y);

            void update(double dt);

        private:

            void yaw(double value);
            void pitch(double value);
            void roll(double value);
            void translate(const osg::Vec3d& offset);

            void stabilize();

            bool mLockUpright;
            bool mFast, mLeft, mRight, mForward, mBackward, mRollLeft, mRollRight;
            osg::Vec3d mUp;
    };

    class OrbitCameraController : public CameraController
    {
        public:

            OrbitCameraController();

            bool handleKeyEvent(QKeyEvent* event, bool pressed);
            bool handleMouseMoveEvent(std::string mode, int x, int y);

            void update(double dt);

        private:

            void onActivate();

            void initialize();

            void rotateHorizontal(double value);
            void rotateVertical(double value);
            void roll(double value);
            void translate(const osg::Vec3d& offset);
            void zoom(double value);

            void lookAtCenter();

            bool mInitialized;
            bool mFast, mLeft, mRight, mUp, mDown, mRollLeft, mRollRight;
            osg::Vec3d mCenter;
    };
}

#endif
