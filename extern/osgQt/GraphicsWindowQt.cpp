/* -*-c++-*- OpenSceneGraph - Copyright (C) 2009 Wang Rui
 *
 * This library is open source and may be redistributed and/or modified under
 * the terms of the OpenSceneGraph Public License (OSGPL) version 0.0 or
 * (at your option) any later version.  The full license is in LICENSE file
 * included with this distribution, and on the openscenegraph.org website.
 *
 * This library is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * OpenSceneGraph Public License for more details.
*/

#include "GraphicsWindowQt"

#include <osg/DeleteHandler>
#include <osgViewer/ViewerBase>
#include <QInputEvent>
#include <QPointer>

#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
#include <QWindow>
#endif

using namespace osgQt;

#if (QT_VERSION < QT_VERSION_CHECK(5, 2, 0))
    #define GETDEVICEPIXELRATIO() 1.0
#else
    #define GETDEVICEPIXELRATIO() devicePixelRatio()
#endif

GLWidget::GLWidget( QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f)
: QGLWidget(parent, shareWidget, f), _gw( NULL )
{
    _devicePixelRatio = GETDEVICEPIXELRATIO();
}

GLWidget::GLWidget( QGLContext* context, QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f)
: QGLWidget(context, parent, shareWidget, f), _gw( NULL )
{
    _devicePixelRatio = GETDEVICEPIXELRATIO();
}

GLWidget::GLWidget( const QGLFormat& format, QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f)
: QGLWidget(format, parent, shareWidget, f), _gw( NULL )
{
    _devicePixelRatio = GETDEVICEPIXELRATIO();
}

GLWidget::~GLWidget()
{
    // close GraphicsWindowQt and remove the reference to us
    if( _gw )
    {
        _gw->close();
        _gw->_widget = NULL;
        _gw = NULL;
    }
}

void GLWidget::processDeferredEvents()
{
    QQueue<QEvent::Type> deferredEventQueueCopy;
    {
        QMutexLocker lock(&_deferredEventQueueMutex);
        deferredEventQueueCopy = _deferredEventQueue;
        _eventCompressor.clear();
        _deferredEventQueue.clear();
    }

    while (!deferredEventQueueCopy.isEmpty())
    {
        QEvent event(deferredEventQueueCopy.dequeue());
        QGLWidget::event(&event);
    }
}

bool GLWidget::event( QEvent* event )
{

    // QEvent::Hide
    //
    // workaround "Qt-workaround" that does glFinish before hiding the widget
    // (the Qt workaround was seen at least in Qt 4.6.3 and 4.7.0)
    //
    // Qt makes the context current, performs glFinish, and releases the context.
    // This makes the problem in OSG multithreaded environment as the context
    // is active in another thread, thus it can not be made current for the purpose
    // of glFinish in this thread.

    // QEvent::ParentChange
    //
    // Reparenting GLWidget may create a new underlying window and a new GL context.
    // Qt will then call doneCurrent on the GL context about to be deleted. The thread
    // where old GL context was current has no longer current context to render to and
    // we cannot make new GL context current in this thread.

    // We workaround above problems by deferring execution of problematic event requests.
    // These events has to be enqueue and executed later in a main GUI thread (GUI operations
    // outside the main thread are not allowed) just before makeCurrent is called from the
    // right thread. The good place for doing that is right after swap in a swapBuffersImplementation.

    if (event->type() == QEvent::Hide)
    {
        // enqueue only the last of QEvent::Hide and QEvent::Show
        enqueueDeferredEvent(QEvent::Hide, QEvent::Show);
        return true;
    }
    else if (event->type() == QEvent::Show)
    {
        // enqueue only the last of QEvent::Show or QEvent::Hide
        enqueueDeferredEvent(QEvent::Show, QEvent::Hide);
        return true;
    }
    else if (event->type() == QEvent::ParentChange)
    {
        // enqueue only the last QEvent::ParentChange
        enqueueDeferredEvent(QEvent::ParentChange);
        return true;
    }
#if QT_VERSION >= QT_VERSION_CHECK(5, 5, 0)
    else if (event->type() == QEvent::PlatformSurface && static_cast<QPlatformSurfaceEvent*>(event)->surfaceEventType() == QPlatformSurfaceEvent::SurfaceAboutToBeDestroyed)
    {
        if (_gw)
            _gw->close();
    }
#endif

    // perform regular event handling
    return QGLWidget::event( event );
}

void GLWidget::resizeEvent( QResizeEvent* event )
{
    if (_gw == nullptr || !_gw->valid())
        return;
    const QSize& size = event->size();

    int scaled_width = static_cast<int>(size.width()*_devicePixelRatio);
    int scaled_height = static_cast<int>(size.height()*_devicePixelRatio);
    _gw->resized( x(), y(), scaled_width,  scaled_height);
    _gw->getEventQueue()->windowResize( x(), y(), scaled_width, scaled_height );
    _gw->requestRedraw();
}

void GLWidget::moveEvent( QMoveEvent* event )
{
    if (_gw == nullptr || !_gw->valid())
        return;
    const QPoint& pos = event->pos();
    int scaled_width = static_cast<int>(width()*_devicePixelRatio);
    int scaled_height = static_cast<int>(height()*_devicePixelRatio);
    _gw->resized( pos.x(), pos.y(), scaled_width,  scaled_height );
    _gw->getEventQueue()->windowResize( pos.x(), pos.y(), scaled_width,  scaled_height );
}

void GLWidget::glDraw()
{
    _gw->requestRedraw();
}

GraphicsWindowQt::GraphicsWindowQt( osg::GraphicsContext::Traits* traits, QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f )
:   _realized(false)
{

    _widget = NULL;
    _traits = traits;
    init( parent, shareWidget, f );
}

GraphicsWindowQt::GraphicsWindowQt( GLWidget* widget )
:   _realized(false)
{
    _widget = widget;
    _traits = _widget ? createTraits( _widget ) : new osg::GraphicsContext::Traits;
    init( NULL, NULL, 0 );
}

GraphicsWindowQt::~GraphicsWindowQt()
{
    close();

    // remove reference from GLWidget
    if ( _widget )
        _widget->_gw = NULL;
}

bool GraphicsWindowQt::init( QWidget* parent, const QGLWidget* shareWidget, Qt::WindowFlags f )
{
    // update _widget and parent by WindowData
    WindowData* windowData = _traits.get() ? dynamic_cast<WindowData*>(_traits->inheritedWindowData.get()) : 0;
    if ( !_widget )
        _widget = windowData ? windowData->_widget : NULL;
    if ( !parent )
        parent = windowData ? windowData->_parent : NULL;

    // create widget if it does not exist
    _ownsWidget = _widget == NULL;
    if ( !_widget )
    {
        // shareWidget
        if ( !shareWidget ) {
            GraphicsWindowQt* sharedContextQt = dynamic_cast<GraphicsWindowQt*>(_traits->sharedContext.get());
            if ( sharedContextQt )
                shareWidget = sharedContextQt->getGLWidget();
        }

        // WindowFlags
        Qt::WindowFlags flags = f | Qt::Window | Qt::CustomizeWindowHint;
        if ( _traits->windowDecoration )
            flags |= Qt::WindowTitleHint | Qt::WindowMinMaxButtonsHint | Qt::WindowSystemMenuHint
#if (QT_VERSION_CHECK(4, 5, 0) <= QT_VERSION)
                | Qt::WindowCloseButtonHint
#endif
                ;

        // create widget
        _widget = new GLWidget( traits2qglFormat( _traits.get() ), parent, shareWidget, flags );
    }

    // set widget name and position
    // (do not set it when we inherited the widget)
    if ( _ownsWidget )
    {
        _widget->setWindowTitle( _traits->windowName.c_str() );
        _widget->move( _traits->x, _traits->y );
        if ( !_traits->supportsResize ) _widget->setFixedSize( _traits->width, _traits->height );
        else _widget->resize( _traits->width, _traits->height );
    }

    // initialize widget properties
    _widget->setAutoBufferSwap( false );
    _widget->setMouseTracking( true );
    _widget->setGraphicsWindow( this );
    useCursor( _traits->useCursor );

    // initialize State
    setState( new osg::State );
    getState()->setGraphicsContext(this);

    // initialize contextID
    if ( _traits.valid() && _traits->sharedContext.valid() )
    {
        getState()->setContextID( _traits->sharedContext->getState()->getContextID() );
        incrementContextIDUsageCount( getState()->getContextID() );
    }
    else
    {
        getState()->setContextID( osg::GraphicsContext::createNewContextID() );
    }

    // make sure the event queue has the correct window rectangle size and input range
    getEventQueue()->syncWindowRectangleWithGraphicsContext();

    return true;
}

QGLFormat GraphicsWindowQt::traits2qglFormat( const osg::GraphicsContext::Traits* traits )
{
    QGLFormat format( QGLFormat::defaultFormat() );

    format.setAlphaBufferSize( traits->alpha );
    format.setRedBufferSize( traits->red );
    format.setGreenBufferSize( traits->green );
    format.setBlueBufferSize( traits->blue );
    format.setDepthBufferSize( traits->depth );
    format.setStencilBufferSize( traits->stencil );
    format.setSampleBuffers( traits->sampleBuffers );
    format.setSamples( traits->samples );

    format.setAlpha( traits->alpha>0 );
    format.setDepth( traits->depth>0 );
    format.setStencil( traits->stencil>0 );
    format.setDoubleBuffer( traits->doubleBuffer );
    format.setSwapInterval( traits->vsync ? 1 : 0 );
    format.setStereo( traits->quadBufferStereo ? 1 : 0 );

    return format;
}

void GraphicsWindowQt::qglFormat2traits( const QGLFormat& format, osg::GraphicsContext::Traits* traits )
{
    traits->red = format.redBufferSize();
    traits->green = format.greenBufferSize();
    traits->blue = format.blueBufferSize();
    traits->alpha = format.alpha() ? format.alphaBufferSize() : 0;
    traits->depth = format.depth() ? format.depthBufferSize() : 0;
    traits->stencil = format.stencil() ? format.stencilBufferSize() : 0;

    traits->sampleBuffers = format.sampleBuffers() ? 1 : 0;
    traits->samples = format.samples();

    traits->quadBufferStereo = format.stereo();
    traits->doubleBuffer = format.doubleBuffer();

    traits->vsync = format.swapInterval() >= 1;
}

osg::GraphicsContext::Traits* GraphicsWindowQt::createTraits( const QGLWidget* widget )
{
    osg::GraphicsContext::Traits *traits = new osg::GraphicsContext::Traits;

    qglFormat2traits( widget->format(), traits );

    QRect r = widget->geometry();
    traits->x = r.x();
    traits->y = r.y();
    traits->width = r.width();
    traits->height = r.height();

    traits->windowName = widget->windowTitle().toLocal8Bit().data();
    Qt::WindowFlags f = widget->windowFlags();
    traits->windowDecoration = ( f & Qt::WindowTitleHint ) &&
                            ( f & Qt::WindowMinMaxButtonsHint ) &&
                            ( f & Qt::WindowSystemMenuHint );
    QSizePolicy sp = widget->sizePolicy();
    traits->supportsResize = sp.horizontalPolicy() != QSizePolicy::Fixed ||
                            sp.verticalPolicy() != QSizePolicy::Fixed;

    return traits;
}

bool GraphicsWindowQt::setWindowRectangleImplementation( int x, int y, int width, int height )
{
    if ( _widget == NULL )
        return false;

    _widget->setGeometry( x, y, width, height );
    return true;
}

void GraphicsWindowQt::getWindowRectangle( int& x, int& y, int& width, int& height )
{
    if ( _widget )
    {
        const QRect& geom = _widget->geometry();
        x = geom.x();
        y = geom.y();
        width = geom.width();
        height = geom.height();
    }
}

bool GraphicsWindowQt::setWindowDecorationImplementation( bool windowDecoration )
{
    Qt::WindowFlags flags = Qt::Window|Qt::CustomizeWindowHint;//|Qt::WindowStaysOnTopHint;
    if ( windowDecoration )
        flags |= Qt::WindowTitleHint|Qt::WindowMinMaxButtonsHint|Qt::WindowSystemMenuHint;
    _traits->windowDecoration = windowDecoration;

    if ( _widget )
    {
        _widget->setWindowFlags( flags );

        return true;
    }

    return false;
}

bool GraphicsWindowQt::getWindowDecoration() const
{
    return _traits->windowDecoration;
}

void GraphicsWindowQt::grabFocus()
{
    if ( _widget )
        _widget->setFocus( Qt::ActiveWindowFocusReason );
}

void GraphicsWindowQt::grabFocusIfPointerInWindow()
{
    if ( _widget->underMouse() )
        _widget->setFocus( Qt::ActiveWindowFocusReason );
}

void GraphicsWindowQt::raiseWindow()
{
    if ( _widget )
        _widget->raise();
}

void GraphicsWindowQt::setWindowName( const std::string& name )
{
    if ( _widget )
        _widget->setWindowTitle( name.c_str() );
}

std::string GraphicsWindowQt::getWindowName()
{
    return _widget ? _widget->windowTitle().toStdString() : "";
}

void GraphicsWindowQt::useCursor( bool cursorOn )
{
    if ( _widget )
    {
        _traits->useCursor = cursorOn;
        if ( !cursorOn ) _widget->setCursor( Qt::BlankCursor );
        else _widget->setCursor( _currentCursor );
    }
}

void GraphicsWindowQt::setCursor( MouseCursor cursor )
{
    if ( cursor==InheritCursor && _widget )
    {
        _widget->unsetCursor();
    }

    switch ( cursor )
    {
    case NoCursor: _currentCursor = Qt::BlankCursor; break;
    case RightArrowCursor: case LeftArrowCursor: _currentCursor = Qt::ArrowCursor; break;
    case InfoCursor: _currentCursor = Qt::SizeAllCursor; break;
    case DestroyCursor: _currentCursor = Qt::ForbiddenCursor; break;
    case HelpCursor: _currentCursor = Qt::WhatsThisCursor; break;
    case CycleCursor: _currentCursor = Qt::ForbiddenCursor; break;
    case SprayCursor: _currentCursor = Qt::SizeAllCursor; break;
    case WaitCursor: _currentCursor = Qt::WaitCursor; break;
    case TextCursor: _currentCursor = Qt::IBeamCursor; break;
    case CrosshairCursor: _currentCursor = Qt::CrossCursor; break;
    case HandCursor: _currentCursor = Qt::OpenHandCursor; break;
    case UpDownCursor: _currentCursor = Qt::SizeVerCursor; break;
    case LeftRightCursor: _currentCursor = Qt::SizeHorCursor; break;
    case TopSideCursor: case BottomSideCursor: _currentCursor = Qt::UpArrowCursor; break;
    case LeftSideCursor: case RightSideCursor: _currentCursor = Qt::SizeHorCursor; break;
    case TopLeftCorner: _currentCursor = Qt::SizeBDiagCursor; break;
    case TopRightCorner: _currentCursor = Qt::SizeFDiagCursor; break;
    case BottomRightCorner: _currentCursor = Qt::SizeBDiagCursor; break;
    case BottomLeftCorner: _currentCursor = Qt::SizeFDiagCursor; break;
    default: break;
    };
    if ( _widget ) _widget->setCursor( _currentCursor );
}

bool GraphicsWindowQt::valid() const
{
    return _widget && _widget->isValid();
}

bool GraphicsWindowQt::realizeImplementation()
{
    // save the current context
    // note: this will save only Qt-based contexts
    const QGLContext *savedContext = QGLContext::currentContext();

    // initialize GL context for the widget
    if ( !valid() )
        _widget->glInit();

    // make current
    _realized = true;
    bool result = makeCurrent();
    _realized = false;

    // fail if we do not have current context
    if ( !result )
    {
        if ( savedContext )
            const_cast< QGLContext* >( savedContext )->makeCurrent();

        OSG_WARN << "Window realize: Can make context current." << std::endl;
        return false;
    }

    _realized = true;

    // make sure the event queue has the correct window rectangle size and input range
    getEventQueue()->syncWindowRectangleWithGraphicsContext();

    // make this window's context not current
    // note: this must be done as we will probably make the context current from another thread
    //       and it is not allowed to have one context current in two threads
    if( !releaseContext() )
        OSG_WARN << "Window realize: Can not release context." << std::endl;

    // restore previous context
    if ( savedContext )
        const_cast< QGLContext* >( savedContext )->makeCurrent();

    return true;
}

bool GraphicsWindowQt::isRealizedImplementation() const
{
    return _realized;
}

void GraphicsWindowQt::closeImplementation()
{
    if ( _widget )
        _widget->close();
    _realized = false;
}

void GraphicsWindowQt::runOperations()
{
    // While in graphics thread this is last chance to do something useful before
    // graphics thread will execute its operations.
    if (_widget->getNumDeferredEvents() > 0)
        _widget->processDeferredEvents();

    if (QGLContext::currentContext() != _widget->context())
        _widget->makeCurrent();

    GraphicsWindow::runOperations();
}

bool GraphicsWindowQt::makeCurrentImplementation()
{
    if (_widget->getNumDeferredEvents() > 0)
        _widget->processDeferredEvents();

    _widget->makeCurrent();

    return true;
}

bool GraphicsWindowQt::releaseContextImplementation()
{
    _widget->doneCurrent();
    return true;
}

void GraphicsWindowQt::swapBuffersImplementation()
{
#if (QT_VERSION >= QT_VERSION_CHECK(5, 0, 0))
    // QOpenGLContext complains if we swap on an non-exposed QWindow
    if (!_widget || !_widget->windowHandle()->isExposed())
        return;
#endif
    // FIXME: the processDeferredEvents should really be executed in a GUI (main) thread context but
    // I couln't find any reliable way to do this. For now, lets hope non of *GUI thread only operations* will
    // be executed in a QGLWidget::event handler. On the other hand, calling GUI only operations in the
    // QGLWidget event handler is an indication of a Qt bug.
    if (_widget->getNumDeferredEvents() > 0)
        _widget->processDeferredEvents();

    // We need to call makeCurrent here to restore our previously current context
    // which may be changed by the processDeferredEvents function.
    _widget->makeCurrent();
    _widget->swapBuffers();
}

void GraphicsWindowQt::requestWarpPointer( float x, float y )
{
    if ( _widget )
        QCursor::setPos( _widget->mapToGlobal(QPoint((int)x,(int)y)) );
}


class QtWindowingSystem : public osg::GraphicsContext::WindowingSystemInterface
{
public:

    QtWindowingSystem()
    {
        OSG_INFO << "QtWindowingSystemInterface()" << std::endl;
    }

    ~QtWindowingSystem()
    {
        if (osg::Referenced::getDeleteHandler())
        {
            osg::Referenced::getDeleteHandler()->setNumFramesToRetainObjects(0);
            osg::Referenced::getDeleteHandler()->flushAll();
        }
    }

    // Access the Qt windowing system through this singleton class.
    static QtWindowingSystem* getInterface()
    {
        static QtWindowingSystem* qtInterface = new QtWindowingSystem;
        return qtInterface;
    }

    // Return the number of screens present in the system
    virtual unsigned int getNumScreens( const osg::GraphicsContext::ScreenIdentifier& /*si*/ )
    {
        OSG_WARN << "osgQt: getNumScreens() not implemented yet." << std::endl;
        return 0;
    }

    // Return the resolution of specified screen
    // (0,0) is returned if screen is unknown
    virtual void getScreenSettings( const osg::GraphicsContext::ScreenIdentifier& /*si*/, osg::GraphicsContext::ScreenSettings & /*resolution*/ )
    {
        OSG_WARN << "osgQt: getScreenSettings() not implemented yet." << std::endl;
    }

    // Set the resolution for given screen
    virtual bool setScreenSettings( const osg::GraphicsContext::ScreenIdentifier& /*si*/, const osg::GraphicsContext::ScreenSettings & /*resolution*/ )
    {
        OSG_WARN << "osgQt: setScreenSettings() not implemented yet." << std::endl;
        return false;
    }

    // Enumerates available resolutions
    virtual void enumerateScreenSettings( const osg::GraphicsContext::ScreenIdentifier& /*screenIdentifier*/, osg::GraphicsContext::ScreenSettingsList & /*resolution*/ )
    {
        OSG_WARN << "osgQt: enumerateScreenSettings() not implemented yet." << std::endl;
    }

    // Create a graphics context with given traits
    virtual osg::GraphicsContext* createGraphicsContext( osg::GraphicsContext::Traits* traits )
    {
        if (traits->pbuffer)
        {
            OSG_WARN << "osgQt: createGraphicsContext - pbuffer not implemented yet." << std::endl;
            return NULL;
        }
        else
        {
            osg::ref_ptr< GraphicsWindowQt > window = new GraphicsWindowQt( traits );
            if (window->valid()) return window.release();
            else return NULL;
        }
    }

private:

    // No implementation for these
    QtWindowingSystem( const QtWindowingSystem& );
    QtWindowingSystem& operator=( const QtWindowingSystem& );
};

