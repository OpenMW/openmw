#include "CompositeOsgRenderer.hpp"
#include "osgQOpenGLWidget.hpp"

#include <mutex>
#include <osgViewer/View>

#include <thread>

CompositeOsgRenderer::CompositeOsgRenderer(QObject* parent)
    : QObject(parent), osgViewer::CompositeViewer(), mSimulationTime(0.0)
{
    _firstFrame = true;
}

CompositeOsgRenderer::CompositeOsgRenderer(osg::ArgumentParser* arguments, QObject* parent)
    : QObject(parent), osgViewer::CompositeViewer(*arguments), mSimulationTime(0.0)
{
    _firstFrame = true;
}

CompositeOsgRenderer::~CompositeOsgRenderer()
{
}

void CompositeOsgRenderer::update()
{
    double dt = mFrameTimer.time_s();
    mFrameTimer.setStartTick();

    emit simulationUpdated(dt);

    mSimulationTime += dt;
    osgQOpenGLWidget* osgWidget = dynamic_cast<osgQOpenGLWidget*>(parent());
    if (osgWidget)
    {
        osgWidget->update();
    }

    double minFrameTime = _runMaxFrameRate > 0.0 ? 1.0 / _runMaxFrameRate : 0.0;
    if (dt < minFrameTime)
    {
        std::this_thread::sleep_for(std::chrono::duration<double>(minFrameTime - dt));
    }
}

void CompositeOsgRenderer::resize(int windowWidth, int windowHeight)
{
    if(!m_osgInitialized)
        return;

    for(RefViews::iterator itr = _views.begin();
        itr != _views.end();
        ++itr)
    {
        osgViewer::View* view = itr->get();
        if(view)
        {
            m_osgWinEmb->resized(0, 0, windowWidth, windowHeight);
            m_osgWinEmb->getEventQueue()->windowResize(0, 0, windowWidth, windowHeight);
       }
   }

    update();
}

void CompositeOsgRenderer::setGraphicsWindowEmbedded(osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> osgWinEmb)
{
    m_osgWinEmb = osgWinEmb;
}

void CompositeOsgRenderer::setupOSG()
{
    m_osgInitialized = true;

    m_osgWinEmb->getEventQueue()->syncWindowRectangleWithGraphicsContext();
    // disable key event (default is Escape key) that the viewer checks on each
    // frame to see
    // if the viewer's done flag should be set to signal end of viewers main
    // loop.
    setKeyEventSetsDone(0);
    setReleaseContextAtEndOfFrameHint(false);
    setThreadingModel(osgViewer::CompositeViewer::SingleThreaded);

    setRunFrameScheme(osgViewer::ViewerBase::ON_DEMAND);

    connect( &mTimer, SIGNAL(timeout()), this, SLOT(update()) );
    mTimer.start( 10 );

    osgViewer::CompositeViewer::Windows windows;
    getWindows(windows);
}

// called from ViewerWidget paintGL() method
void CompositeOsgRenderer::frame(double simulationTime)
{
    if(_done) return;

    if(_firstFrame)
    {
        viewerInit();
        realize();
        _firstFrame = false;
    }

    advance(simulationTime);

    eventTraversal();
    updateTraversal();
    renderingTraversals();
}

void CompositeOsgRenderer::timerEvent(QTimerEvent* /*event*/)
{
    // ask ViewerWidget to update 3D view
    if(getRunFrameScheme() != osgViewer::ViewerBase::ON_DEMAND || checkNeedToDoFrame())
    {
        update();
    }
}
