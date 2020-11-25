#include "osgQOpenGLWidget"
#include "OSGRenderer"

#include <iostream>

#include <osgViewer/Viewer>
#include <osg/GL>

#include <QApplication>
#include <QKeyEvent>
#include <QInputDialog>
#include <QLayout>
#include <QMainWindow>
#include <QScreen>
#include <QWindow>

osgQOpenGLWidget::osgQOpenGLWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
}

osgQOpenGLWidget::osgQOpenGLWidget(osg::ArgumentParser* arguments,
                                   QWidget* parent) :
    QOpenGLWidget(parent),
    _arguments(arguments)
{

}

osgQOpenGLWidget::~osgQOpenGLWidget()
{
}

osgViewer::Viewer* osgQOpenGLWidget::getOsgViewer()
{
    return m_renderer;
}

OpenThreads::ReadWriteMutex* osgQOpenGLWidget::mutex()
{
    return &_osgMutex;
}


void osgQOpenGLWidget::initializeGL()
{
    // Initializes OpenGL function resolution for the current context.
    std::cout << "osgQOpenGLWidget::initializeGL" << std::endl;
    initializeOpenGLFunctions();
    createRenderer();
    emit initialized();
}

void osgQOpenGLWidget::resizeGL(int w, int h)
{
    Q_ASSERT(m_renderer);
    m_renderer->resize(w, h);
}

void osgQOpenGLWidget::paintGL()
{
    OpenThreads::ScopedReadLock locker(_osgMutex);
	if (_isFirstFrame) {
		_isFirstFrame = false;
		m_renderer->getCamera()->getGraphicsContext()->setDefaultFboId(defaultFramebufferObject());
	}
	m_renderer->frame();
}

void osgQOpenGLWidget::setDefaultDisplaySettings()
{
    osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
    ds->setNvOptimusEnablement(1);
    ds->setStereo(false);
}

void osgQOpenGLWidget::createRenderer()
{
    // call this before creating a View...
    setDefaultDisplaySettings();
	if (!_arguments) {
		m_renderer = new OSGRenderer(this);
	} else {
		m_renderer = new OSGRenderer(_arguments, this);
	}
    int width = 640;
    int height = 480;
    if ( QWidget* widget = dynamic_cast<QWidget*> (parent()) )
    {
        width = widget->width();
        height = widget->height();
    }
    std::cout << "osgQOpenGLWidget::createRenderer() width " << width << " height " << height << std::endl;
    m_renderer->setupOSG(width, height);
}
