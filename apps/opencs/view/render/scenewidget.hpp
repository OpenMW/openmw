#ifndef OPENCS_VIEW_SCENEWIDGET_H
#define OPENCS_VIEW_SCENEWIDGET_H

#include <map>
#include <memory>

#include <QWidget>
#include <QTimer>

#include <osgViewer/View>
#include <osgViewer/CompositeViewer>

#include "lightingday.hpp"
#include "lightingnight.hpp"
#include "lightingbright.hpp"


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
            RenderWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
            virtual ~RenderWidget();

            /// Initiates a request to redraw the view
            void flagAsModified();

            void setVisibilityMask(unsigned int mask);

            osg::Camera *getCamera();

        protected:

            osg::ref_ptr<osgViewer::View> mView;
            osg::ref_ptr<osg::Group> mRootNode;

            void updateCameraParameters(double overrideAspect = -1.0);

            QTimer mTimer;

        protected slots:

            void toggleRenderStats();
    };

    /// Extension of RenderWidget to support lighting mode selection & toolbar
    class SceneWidget : public RenderWidget
    {
            Q_OBJECT
        public:
            SceneWidget(std::shared_ptr<Resource::ResourceSystem> resourceSystem, QWidget* parent = nullptr,
                        Qt::WindowFlags f = Qt::WindowFlags(), bool retrieveInput = true);
            virtual ~SceneWidget();

            CSVWidget::SceneToolMode *makeLightingSelector (CSVWidget::SceneToolbar *parent);
            ///< \attention The created tool is not added to the toolbar (via addTool). Doing that
            /// is the responsibility of the calling function.

            void setDefaultAmbient (const osg::Vec4f& colour);
            ///< \note The actual ambient colour may differ based on lighting settings.

            void setExterior (bool isExterior);

        protected:
            void setLighting (Lighting *lighting);
            ///< \attention The ownership of \a lighting is not transferred to *this.

            void setAmbient(const osg::Vec4f& ambient);

            void mouseMoveEvent (QMouseEvent *event) override;
            void wheelEvent (QWheelEvent *event) override;

            osg::ref_ptr<osg::Geometry> createGradientRectangle(QColor bgColour, QColor gradientColour);
            osg::ref_ptr<osg::Camera> createGradientCamera(QColor bgColour, QColor gradientColour);
            void updateGradientCamera(QColor bgColour, QColor gradientColour);

            std::shared_ptr<Resource::ResourceSystem> mResourceSystem;

            Lighting* mLighting;
            
            osg::ref_ptr<osg::Camera> mGradientCamera;
            osg::Vec4f mDefaultAmbient;
            bool mHasDefaultAmbient;
            bool mIsExterior;
            LightingDay mLightingDay;
            LightingNight mLightingNight;
            LightingBright mLightingBright;

            int mPrevMouseX, mPrevMouseY;
            
            /// Tells update that camera isn't set
            bool mCamPositionSet;

            FreeCameraController* mFreeCamControl;
            OrbitCameraController* mOrbitCamControl;
            CameraController* mCurrentCamControl;

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
