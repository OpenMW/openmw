#include "CompositeOsgRenderer.hpp"
#include "osgQOpenGLWidget.hpp"

#include <mutex>
#include <osgViewer/View>

#include <QApplication>
#include <QScreen>
#include <QWindow>

osgQOpenGLWidget::osgQOpenGLWidget(QWidget* parent)
    : QOpenGLWidget(parent)
{
    m_renderer = new CompositeOsgRenderer(this);
    setMouseTracking(true);
}

osgQOpenGLWidget::osgQOpenGLWidget(osg::ArgumentParser* arguments,
                                   QWidget* parent) :
    QOpenGLWidget(parent),
    _arguments(arguments)
{
    m_renderer = new CompositeOsgRenderer(this);
}

osgQOpenGLWidget::~osgQOpenGLWidget()
{
    if (m_renderer) delete m_renderer;
}

osgViewer::View* osgQOpenGLWidget::getOsgView(unsigned i)
{
    if (m_renderer) return m_renderer->getView(i);
    else return nullptr;
}

std::mutex* osgQOpenGLWidget::mutex()
{
    return &_osgMutex;
}

void osgQOpenGLWidget::initializeGL()
{
    createRenderer();
    emit initialized();
}

void osgQOpenGLWidget::resizeGL(int w, int h)
{
    Q_ASSERT(m_renderer);
    QScreen* screen = windowHandle()
                      && windowHandle()->screen() ? windowHandle()->screen() :
                      qApp->screens().front();
    m_renderer->resize(w * screen->devicePixelRatio(), h * screen->devicePixelRatio());
}

void osgQOpenGLWidget::paintGL()
{
    std::scoped_lock locker(_osgMutex);
    if (_isFirstFrame)
    {
        _isFirstFrame = false;
        for (unsigned int i = 0; i < m_renderer->getNumViews(); ++i)
        {
            m_renderer->getView(i)->getCamera()->getGraphicsContext()->setDefaultFboId(defaultFramebufferObject());
        }
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
    if (m_renderer) return m_renderer;
    return nullptr;
}

void osgQOpenGLWidget::setGraphicsWindowEmbedded(osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> osgWinEmb)
{
    if (m_renderer) m_renderer->setGraphicsWindowEmbedded(std::move(osgWinEmb));
}

void osgQOpenGLWidget::createRenderer()
{
    // call this before creating a View...
    setDefaultDisplaySettings();
    if (!m_renderer) m_renderer = new CompositeOsgRenderer(this);

    m_renderer->setupOSG();
}
