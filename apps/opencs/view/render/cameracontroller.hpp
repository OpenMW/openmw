#ifndef OPENCS_VIEW_CAMERACONTROLLER_H
#define OPENCS_VIEW_CAMERACONTROLLER_H

#include <string>

#include <osg/ref_ptr>
#include <osg/Vec3d>

class QKeyEvent;

namespace osg
{
    class Camera;
    class Group;
}

namespace CSVRender
{
    class CameraController
    {
        public:

            static const osg::Vec3d WorldUp;

            static const osg::Vec3d LocalUp;
            static const osg::Vec3d LocalLeft;
            static const osg::Vec3d LocalForward;

            CameraController();
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

            virtual bool handleKeyEvent(QKeyEvent* event, bool pressed) = 0;
            virtual bool handleMouseMoveEvent(std::string mode, int x, int y) = 0;

            virtual void update(double dt) = 0;

            virtual void resetInput() = 0;

        protected:

            virtual void onActivate(){}

        private:

            bool mActive, mInverted;
            double mCameraSensitivity;
            double mSecondaryMoveMult;
            double mWheelMoveMult;

            osg::Camera* mCamera;
    };

    class FreeCameraController : public CameraController
    {
        public:

            FreeCameraController();

            double getLinearSpeed() const;
            double getRotationalSpeed() const;
            double getSpeedMultiplier() const;

            void setLinearSpeed(double value);
            void setRotationalSpeed(double value);
            void setSpeedMultiplier(double value);

            void fixUpAxis(const osg::Vec3d& up);
            void unfixUpAxis();

            bool handleKeyEvent(QKeyEvent* event, bool pressed);
            bool handleMouseMoveEvent(std::string mode, int x, int y);

            void update(double dt);

            void resetInput();

        private:

            void yaw(double value);
            void pitch(double value);
            void roll(double value);
            void translate(const osg::Vec3d& offset);

            void stabilize();

            bool mLockUpright, mModified;
            bool mFast, mLeft, mRight, mForward, mBackward, mRollLeft, mRollRight;
            osg::Vec3d mUp;

            double mLinSpeed;
            double mRotSpeed;
            double mSpeedMult;
    };

    class OrbitCameraController : public CameraController
    {
        public:

            OrbitCameraController();

            osg::Vec3d getCenter() const;
            double getOrbitSpeed() const;
            double getOrbitSpeedMultiplier() const;
            unsigned int getPickingMask() const;

            void setCenter(const osg::Vec3d& center);
            void setOrbitSpeed(double value);
            void setOrbitSpeedMultiplier(double value);
            void setPickingMask(unsigned int value);

            bool handleKeyEvent(QKeyEvent* event, bool pressed);
            bool handleMouseMoveEvent(std::string mode, int x, int y);

            void update(double dt);

            void resetInput();

        private:

            void onActivate();

            void initialize();

            void rotateHorizontal(double value);
            void rotateVertical(double value);
            void roll(double value);
            void translate(const osg::Vec3d& offset);
            void zoom(double value);

            bool mInitialized;
            bool mFast, mLeft, mRight, mUp, mDown, mRollLeft, mRollRight;
            unsigned int mPickingMask;
            osg::Vec3d mCenter;
            double mDistance;

            double mOrbitSpeed;
            double mOrbitSpeedMult;
    };
}

#endif
