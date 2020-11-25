#include "osgQOpenGLWidget"
#include "CompositeOsgRenderer"

#include <iostream>

#include <osgViewer/View>
#include <osgViewer/CompositeViewer>
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

osgViewer::View* osgQOpenGLWidget::getOsgView(unsigned i)
{
    if (m_renderer) return m_renderer->getView(i);
    else return nullptr;
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
		//m_renderer->getView(?)->getCamera()->getGraphicsContext()->setDefaultFboId(defaultFramebufferObject());
	}
	m_renderer->frame();
}

void osgQOpenGLWidget::setDefaultDisplaySettings()
{
    osg::DisplaySettings* ds = osg::DisplaySettings::instance().get();
    ds->setNvOptimusEnablement(1);
    ds->setStereo(false);
}

CompositeOsgRenderer* osgQOpenGLWidget::getCompositeViewer()
{
    return m_renderer;
}

void osgQOpenGLWidget::setGraphicsWindowEmbedded(osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> osgWinEmb)
{
    if (!m_renderer)
    {
        std::cout << "osgQOpenGLWidget::setGraphicsWindowEmbedded creating m_renderer " << std::endl;
        m_renderer = new CompositeOsgRenderer(this);
    }
    if (m_renderer) m_renderer->setGraphicsWindowEmbedded(osgWinEmb);
}

void osgQOpenGLWidget::createRenderer()
{
    // call this before creating a View...
    setDefaultDisplaySettings();
	if (!m_renderer) m_renderer = new CompositeOsgRenderer(this);

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
