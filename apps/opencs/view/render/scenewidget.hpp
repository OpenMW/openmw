#ifndef OPENCS_VIEW_SCENEWIDGET_H
#define OPENCS_VIEW_SCENEWIDGET_H

#include <map>
#include <memory>
#include <mutex>

#ifdef __APPLE__
#   define __glext_h_
#   include <QtGui/qopengl.h>
#   undef __glext_h_
#   include <QtGui/qopenglext.h>
#endif

#ifdef WIN32
//#define __gl_h_
#include <osg/GL>
#endif

#include <QOpenGLWidget>
#include <QOpenGLFunctions>
#include <QWidget>
#include <QTimer>

#include <osgViewer/View>
#include <osgViewer/CompositeViewer>

#include "lightingday.hpp"
#include "lightingnight.hpp"
#include "lightingbright.hpp"

class CompositeOsgRenderer;

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

    class RenderWidget : public QOpenGLWidget, protected QOpenGLFunctions
    {
            Q_OBJECT

        public:
            RenderWidget(QWidget* parent = nullptr, Qt::WindowFlags f = Qt::WindowFlags());
            virtual ~RenderWidget();

            virtual std::mutex* mutex();

            /// Initiates a request to redraw the view
            void flagAsModified();

            void setVisibilityMask(int mask);

            osg::Camera *getCamera();

        protected:

            CompositeOsgRenderer* mRenderer;
            osg::ref_ptr<osgViewer::View> mView;
            osg::ref_ptr<osg::Group> mRootNode;
            std::mutex _osgMutex;
            bool _isFirstFrame {true};

            void initializeGL() override;
            void resizeGL(int w, int h) override;

            //! lock scene graph and call osgViewer::frame()
            void paintGL() override;

            void updateCameraParameters(double overrideAspect = -1.0);

            QTimer mTimer;

        protected slots:

            void toggleRenderStats();

        signals:
             void initialized();
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

            std::shared_ptr<Resource::ResourceSystem> mResourceSystem;

            Lighting* mLighting;

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
}

#endif
