#ifndef OSGQOPENGLWINDOW_H
#define OSGQOPENGLWINDOW_H

#ifdef __APPLE__
#   define __glext_h_
#   include <QtGui/qopengl.h>
#   undef __glext_h_
#   include <QtGui/qopenglext.h>
#endif

#include <OpenThreads/ReadWriteMutex>

#ifdef WIN32
//#define __gl_h_
#include <osg/GL>
#endif

#include <QOpenGLWindow>
#include <QOpenGLFunctions>
#include <QReadWriteLock>

class OSGRenderer;
class QWidget;

namespace osgViewer
{
    class Viewer;
}

class osgQOpenGLWindow : public QOpenGLWindow,
    protected QOpenGLFunctions
{
    Q_OBJECT

protected:
    OSGRenderer* m_renderer {nullptr};
    bool _osgWantsToRenderFrame{true};
    OpenThreads::ReadWriteMutex _osgMutex;
	bool _isFirstFrame {true};
    friend class OSGRenderer;

    QWidget* _widget = nullptr;

public:
    osgQOpenGLWindow(QWidget* parent = nullptr);
    virtual ~osgQOpenGLWindow();

    /** Get osgViewer View */
    virtual osgViewer::Viewer* getOsgViewer();

    //! get mutex
    virtual OpenThreads::ReadWriteMutex* mutex();

    QWidget* asWidget()
    {
        return _widget;
    }

signals:
    void initialized();

protected:

    //! call createRender. If overloaded, this method must send initialized signal at end
    void initializeGL() override;

    void resizeGL(int w, int h) override;

    //! lock scene graph and call osgViewer::frame()
    void paintGL() override;

    //! called before creating renderer
    virtual void setDefaultDisplaySettings();

    void keyPressEvent(QKeyEvent* event) override;
    void keyReleaseEvent(QKeyEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void mouseDoubleClickEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

    void createRenderer();
};

#endif // OSGQOPENGLWINDOW_H
