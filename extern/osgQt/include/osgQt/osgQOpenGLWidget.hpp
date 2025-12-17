#ifndef OSGQOPENGLWIDGET_H
#define OSGQOPENGLWIDGET_H

#include <mutex>

#ifdef WIN32
#include <osg/GL>
#endif

#include <osg/ArgumentParser>

#include <QOpenGLWidget>
#include <QReadWriteLock>

class CompositeOsgRenderer;

namespace osgViewer
{
    class View;
    class GraphicsWindowEmbedded;
}

class osgQOpenGLWidget : public QOpenGLWidget
{
    Q_OBJECT

protected:
    CompositeOsgRenderer* m_renderer {nullptr};
    std::mutex _osgMutex;
    osg::ArgumentParser* _arguments {nullptr};
    bool _isFirstFrame {true};

    friend class CompositeOsgRenderer;

public:
    osgQOpenGLWidget(QWidget* parent = nullptr);
    osgQOpenGLWidget(osg::ArgumentParser* arguments, QWidget* parent = nullptr);
    virtual ~osgQOpenGLWidget();

    /** Get osgViewer View */
    virtual osgViewer::View* getOsgView(unsigned i);

    //! get mutex
    virtual std::mutex* mutex();

    CompositeOsgRenderer* getCompositeViewer();

    void setGraphicsWindowEmbedded(osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> osgWinEmb);

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

    void createRenderer();

    bool event(QEvent* e) override;
};

#endif // OSGQOPENGLWIDGET_H
