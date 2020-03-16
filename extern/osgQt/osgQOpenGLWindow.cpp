#include "osgQOpenGLWindow"
#include "OSGRenderer"

#include <osgViewer/Viewer>
#include <osg/GL>

#include <QApplication>
#include <QKeyEvent>
#include <QInputDialog>
#include <QLayout>
#include <QMainWindow>
#include <QScreen>
#include <QWindow>

osgQOpenGLWindow::osgQOpenGLWindow(QWidget* parent)
    : QOpenGLWindow(QOpenGLWindow::NoPartialUpdate, nullptr)
{
    _widget = QWidget::createWindowContainer(this);
}

osgQOpenGLWindow::~osgQOpenGLWindow()
{
}

osgViewer::Viewer* osgQOpenGLWindow::getOsgViewer()
{
    return m_renderer;
}

OpenThreads::ReadWriteMutex* osgQOpenGLWindow::mutex()
{
    return &_osgMutex;
}


void osgQOpenGLWindow::initializeGL()
{
    // Initializes OpenGL function resolution for the current context.
    initializeOpenGLFunctions();
    createRenderer();
    emit initialized();
}

void osgQOpenGLWindow::resizeGL(int w, int h)
{
    Q_ASSERT(m_renderer);
    double pixelRatio = screen()->devicePixelRatio();
    m_renderer->resize(w, h, pixelRatio);
}

void osgQOpenGLWindow::paintGL()
{
    OpenThreads::ScopedReadLock locker(_osgMutex);
	if (_isFirstFrame) {
		_isFirstFrame = false;
		m_renderer->getCamera()->getGraphicsContext()->setDefaultFboId(defaultFramebufferObject());
	}
    m_renderer->frame();
}

void osgQOpenGLWindow::keyPressEvent(QKeyEvent* event)
{
    Q_ASSERT(m_renderer);
    // forward event to renderer
    m_renderer->keyPressEvent(event);
}

void osgQOpenGLWindow::keyReleaseEvent(QKeyEvent* event)
{
    Q_ASSERT(m_renderer);
    // forward event to renderer
    m_renderer->keyReleaseEvent(event);
}

void osgQOpenGLWindow::mousePressEvent(QMouseEvent* event)
{
    Q_ASSERT(m_renderer);
    // forward event to renderer
    m_renderer->mousePressEvent(event);
}

void osgQOpenGLWindow::mouseReleaseEvent(QMouseEvent* event)
{
    Q_ASSERT(m_renderer);
    // forward event to renderer
    m_renderer->mouseReleaseEvent(event);
}

void osgQOpenGLWindow::mouseDoubleClickEvent(QMouseEvent* event)
{
    Q_ASSERT(m_renderer);
    // forward event to renderer
    m_renderer->mouseDoubleClickEvent(event);
}

void osgQOpenGLWindow::mouseMoveEvent(QMouseEvent* event)
{
    Q_ASSERT(m_renderer);
    // forward event to renderer
    m_renderer->mouseMoveEvent(event);
}

void osgQOpenGLWindow::wheelEvent(QWheelEvent* event)
{
    Q_ASSERT(m_renderer);
    // forward event to renderer
    m_renderer->wheelEvent(event);
}

void osgQOpenGLWindow::setDefaultDisplaySettings()
{
    osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
    ds->setNvOptimusEnablement(1);
    ds->setStereo(false);
}

void osgQOpenGLWindow::createRenderer()
{
    // call this before creating a View...
    setDefaultDisplaySettings();

    m_renderer = new OSGRenderer(this);
    double pixelRatio = screen()->devicePixelRatio();
    m_renderer->setupOSG(width(), height(), pixelRatio);
}
