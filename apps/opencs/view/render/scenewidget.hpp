#ifndef OPENCS_VIEW_SCENEWIDGET_H
#define OPENCS_VIEW_SCENEWIDGET_H

#include <map>
#include <memory>

#include <QWidget>
#include <QTimer>

#include <boost/shared_ptr.hpp>

#include "lightingday.hpp"
#include "lightingnight.hpp"
#include "lightingbright.hpp"

#include <osgViewer/View>
#include <osgViewer/CompositeViewer>

namespace Resource
{
    class ResourceSystem;
}

namespace osg
{
    class Group;
    class Camera;
}

namespace CSVWidget
{
    class SceneToolMode;
    class SceneToolbar;
}

namespace CSMPrefs
{
    class Setting;
}

namespace CSVRender
{
    class CameraController;
    class FreeCameraController;
    class OrbitCameraController;
    class Lighting;

    class RenderWidget : public QWidget
    {
            Q_OBJECT

        public:
            RenderWidget(QWidget* parent = 0, Qt::WindowFlags f = 0);
            virtual ~RenderWidget();

            void flagAsModified();

            void setVisibilityMask(int mask);

            osg::Camera *getCamera();

        protected:

            osg::ref_ptr<osgViewer::View> mView;

            osg::Group* mRootNode;

            QTimer mTimer;
    };

    // Extension of RenderWidget to support lighting mode selection & toolbar
    class SceneWidget : public RenderWidget
    {
            Q_OBJECT
        public:
            SceneWidget(boost::shared_ptr<Resource::ResourceSystem> resourceSystem, QWidget* parent = 0,
                    Qt::WindowFlags f = 0, bool retrieveInput = true);
            virtual ~SceneWidget();

            CSVWidget::SceneToolMode *makeLightingSelector (CSVWidget::SceneToolbar *parent);
            ///< \attention The created tool is not added to the toolbar (via addTool). Doing that
            /// is the responsibility of the calling function.

            void setDefaultAmbient (const osg::Vec4f& colour);
            ///< \note The actual ambient colour may differ based on lighting settings.

        protected:
            void setLighting (Lighting *lighting);
            ///< \attention The ownership of \a lighting is not transferred to *this.

            void setAmbient(const osg::Vec4f& ambient);

            virtual void mousePressEvent (QMouseEvent *event);
            virtual void mouseReleaseEvent (QMouseEvent *event);
            virtual void mouseMoveEvent (QMouseEvent *event);
            virtual void wheelEvent (QWheelEvent *event);
            virtual void keyPressEvent (QKeyEvent *event);
            virtual void keyReleaseEvent (QKeyEvent *event);
            virtual void focusOutEvent (QFocusEvent *event);

            /// \return Is \a key a button mapping setting? (ignored otherwise)
            virtual bool storeMappingSetting (const CSMPrefs::Setting *setting);

            std::string mapButton (QMouseEvent *event);

            boost::shared_ptr<Resource::ResourceSystem> mResourceSystem;

            Lighting* mLighting;

            osg::Vec4f mDefaultAmbient;
            bool mHasDefaultAmbient;
            LightingDay mLightingDay;
            LightingNight mLightingNight;
            LightingBright mLightingBright;

            int mPrevMouseX, mPrevMouseY;
            std::string mMouseMode;
            std::auto_ptr<FreeCameraController> mFreeCamControl;
            std::auto_ptr<OrbitCameraController> mOrbitCamControl;
            CameraController* mCurrentCamControl;

            std::map<std::pair<Qt::MouseButton, bool>, std::string> mButtonMapping;

        private:
            bool mCamPositionSet;

        public slots:
            void update(double dt);

        protected slots:

            virtual void settingChanged (const CSMPrefs::Setting *setting);

            void selectNavigationMode (const std::string& mode);

        private slots:

            void selectLightingMode (const std::string& mode);

        signals:

            void focusToolbarRequest();
    };


    // There are rendering glitches when using multiple Viewer instances, work around using CompositeViewer with multiple views
    class CompositeViewer : public QObject, public osgViewer::CompositeViewer
    {
            Q_OBJECT
        public:
            CompositeViewer();

            static CompositeViewer& get();

            QTimer mTimer;

        private:
            osg::Timer mFrameTimer;
            double mSimulationTime;

        public slots:
            void update();

        signals:
            void simulationUpdated(double dt);
    };

}

#endif
