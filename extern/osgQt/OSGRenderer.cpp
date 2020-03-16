// Copyright (C) 2017 Mike Krus
//
// This program is free software; you can redistribute it and/or
// modify it under the terms of the GNU General Public License as
// published by the Free Software Foundation; either version 2 of the
// License, or (at your option) any later version.
//
// This program is distributed in the hope that it will be useful, but
// WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// General Public License for more details.
//
// You should have received a copy of the GNU General Public License
// along with this program; if not, write to the Free Software
// Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA  02110-1301, USA.

#include "OSGRenderer"

#include "osgQOpenGLWindow"
#include "osgQOpenGLWidget"

//#include <osgQOpenGL/CullVisitorEx>
//#include <osgQOpenGL/GraphicsWindowEx>
#include <QApplication>
#include <QScreen>
#include <QOpenGLContext>
#include <QOpenGLFunctions>
#include <QOpenGLShaderProgram>
#include <QOpenGLVertexArrayObject>

#include <QKeyEvent>
#include <QMouseEvent>
#include <QWheelEvent>

#include <QThread>

namespace
{

    class QtKeyboardMap
    {
    public:
        QtKeyboardMap()
        {
            mKeyMap[Qt::Key_Escape     ] = osgGA::GUIEventAdapter::KEY_Escape;
            mKeyMap[Qt::Key_Delete     ] = osgGA::GUIEventAdapter::KEY_Delete;
            mKeyMap[Qt::Key_Home       ] = osgGA::GUIEventAdapter::KEY_Home;
            mKeyMap[Qt::Key_Enter      ] = osgGA::GUIEventAdapter::KEY_KP_Enter;
            mKeyMap[Qt::Key_End        ] = osgGA::GUIEventAdapter::KEY_End;
            mKeyMap[Qt::Key_Return     ] = osgGA::GUIEventAdapter::KEY_Return;
            mKeyMap[Qt::Key_PageUp     ] = osgGA::GUIEventAdapter::KEY_Page_Up;
            mKeyMap[Qt::Key_PageDown   ] = osgGA::GUIEventAdapter::KEY_Page_Down;
            mKeyMap[Qt::Key_Left       ] = osgGA::GUIEventAdapter::KEY_Left;
            mKeyMap[Qt::Key_Right      ] = osgGA::GUIEventAdapter::KEY_Right;
            mKeyMap[Qt::Key_Up         ] = osgGA::GUIEventAdapter::KEY_Up;
            mKeyMap[Qt::Key_Down       ] = osgGA::GUIEventAdapter::KEY_Down;
            mKeyMap[Qt::Key_Backspace  ] = osgGA::GUIEventAdapter::KEY_BackSpace;
            mKeyMap[Qt::Key_Tab        ] = osgGA::GUIEventAdapter::KEY_Tab;
            mKeyMap[Qt::Key_Space      ] = osgGA::GUIEventAdapter::KEY_Space;
            mKeyMap[Qt::Key_Delete     ] = osgGA::GUIEventAdapter::KEY_Delete;
            mKeyMap[Qt::Key_Alt        ] = osgGA::GUIEventAdapter::KEY_Alt_L;
            mKeyMap[Qt::Key_Shift      ] = osgGA::GUIEventAdapter::KEY_Shift_L;
            mKeyMap[Qt::Key_Control    ] = osgGA::GUIEventAdapter::KEY_Control_L;
            mKeyMap[Qt::Key_Meta       ] = osgGA::GUIEventAdapter::KEY_Meta_L;
            mKeyMap[Qt::Key_F1             ] = osgGA::GUIEventAdapter::KEY_F1;
            mKeyMap[Qt::Key_F2             ] = osgGA::GUIEventAdapter::KEY_F2;
            mKeyMap[Qt::Key_F3             ] = osgGA::GUIEventAdapter::KEY_F3;
            mKeyMap[Qt::Key_F4             ] = osgGA::GUIEventAdapter::KEY_F4;
            mKeyMap[Qt::Key_F5             ] = osgGA::GUIEventAdapter::KEY_F5;
            mKeyMap[Qt::Key_F6             ] = osgGA::GUIEventAdapter::KEY_F6;
            mKeyMap[Qt::Key_F7             ] = osgGA::GUIEventAdapter::KEY_F7;
            mKeyMap[Qt::Key_F8             ] = osgGA::GUIEventAdapter::KEY_F8;
            mKeyMap[Qt::Key_F9             ] = osgGA::GUIEventAdapter::KEY_F9;
            mKeyMap[Qt::Key_F10            ] = osgGA::GUIEventAdapter::KEY_F10;
            mKeyMap[Qt::Key_F11            ] = osgGA::GUIEventAdapter::KEY_F11;
            mKeyMap[Qt::Key_F12            ] = osgGA::GUIEventAdapter::KEY_F12;
            mKeyMap[Qt::Key_F13            ] = osgGA::GUIEventAdapter::KEY_F13;
            mKeyMap[Qt::Key_F14            ] = osgGA::GUIEventAdapter::KEY_F14;
            mKeyMap[Qt::Key_F15            ] = osgGA::GUIEventAdapter::KEY_F15;
            mKeyMap[Qt::Key_F16            ] = osgGA::GUIEventAdapter::KEY_F16;
            mKeyMap[Qt::Key_F17            ] = osgGA::GUIEventAdapter::KEY_F17;
            mKeyMap[Qt::Key_F18            ] = osgGA::GUIEventAdapter::KEY_F18;
            mKeyMap[Qt::Key_F19            ] = osgGA::GUIEventAdapter::KEY_F19;
            mKeyMap[Qt::Key_F20            ] = osgGA::GUIEventAdapter::KEY_F20;
            mKeyMap[Qt::Key_hyphen         ] = '-';
            mKeyMap[Qt::Key_Equal         ] = '=';
            mKeyMap[Qt::Key_division      ] = osgGA::GUIEventAdapter::KEY_KP_Divide;
            mKeyMap[Qt::Key_multiply      ] = osgGA::GUIEventAdapter::KEY_KP_Multiply;
            mKeyMap[Qt::Key_Minus         ] = '-';
            mKeyMap[Qt::Key_Plus          ] = '+';
            mKeyMap[Qt::Key_Insert        ] = osgGA::GUIEventAdapter::KEY_KP_Insert;
        }

        ~QtKeyboardMap()
        {
        }

        int remapKey(QKeyEvent* event)
        {
            KeyMap::iterator itr = mKeyMap.find(event->key());

            if(itr == mKeyMap.end())
            {
                return int(*(event->text().toLatin1().data()));
            }
            else
                return itr->second;
        }

    private:
        typedef std::map<unsigned int, int> KeyMap;
        KeyMap mKeyMap;
    };

    static QtKeyboardMap s_QtKeyboardMap;
} // namespace

OSGRenderer::OSGRenderer(QObject* parent)
    : QObject(parent), osgViewer::Viewer()
{
    //    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
    //                     [this]()
    //    {
    //        _applicationAboutToQuit = true;
    //        killTimer(_timerId);
    //        _timerId = 0;
    //    });
}

OSGRenderer::OSGRenderer(osg::ArgumentParser* arguments, QObject* parent)
    : QObject(parent), osgViewer::Viewer(*arguments)
{
    //    QObject::connect(QCoreApplication::instance(), &QCoreApplication::aboutToQuit,
    //                     [this]()
    //    {
    //        _applicationAboutToQuit = true;
    //        killTimer(_timerId);
    //        _timerId = 0;
    //    });
}

OSGRenderer::~OSGRenderer()
{
}

void OSGRenderer::update()
{
    osgQOpenGLWindow* osgWidgetRendered = dynamic_cast<osgQOpenGLWindow*>(parent());

    if(osgWidgetRendered != nullptr)
    {
        osgWidgetRendered->_osgWantsToRenderFrame = true;
        osgWidgetRendered->update();
    }

    else
    {
        osgQOpenGLWidget* osgWidget = dynamic_cast<osgQOpenGLWidget*>(parent());
        osgWidget->_osgWantsToRenderFrame = true;
        osgWidget->update();
    }
}

void OSGRenderer::resize(int windowWidth, int windowHeight, float windowScale)
{
    if(!m_osgInitialized)
        return;

    m_windowScale = windowScale;

    /*  _camera->setViewport(new osg::Viewport(0, 0, windowWidth * windowScale,
                                           windowHeight * windowScale));*/

    m_osgWinEmb->resized(0, 0,
                         windowWidth * windowScale,
                         windowHeight * windowScale);
    m_osgWinEmb->getEventQueue()->windowResize(0, 0,
                                               windowWidth * windowScale,
                                               windowHeight * windowScale);

    update();
}


void OSGRenderer::setupOSG(int windowWidth, int windowHeight, float windowScale)
{
    m_osgInitialized = true;
    m_windowScale = windowScale;
    m_osgWinEmb = new osgViewer::GraphicsWindowEmbedded(0, 0,
                                                        windowWidth * windowScale, windowHeight * windowScale);
    //m_osgWinEmb = new osgViewer::GraphicsWindowEmbedded(0, 0, windowWidth * windowScale, windowHeight * windowScale);
    // make sure the event queue has the correct window rectangle size and input range
    m_osgWinEmb->getEventQueue()->syncWindowRectangleWithGraphicsContext();
    _camera->setViewport(new osg::Viewport(0, 0, windowWidth * windowScale,
                                           windowHeight * windowScale));
    _camera->setGraphicsContext(m_osgWinEmb.get());
    // disable key event (default is Escape key) that the viewer checks on each
    // frame to see
    // if the viewer's done flag should be set to signal end of viewers main
    // loop.
    setKeyEventSetsDone(0);
    setReleaseContextAtEndOfFrameHint(false);
    setThreadingModel(osgViewer::Viewer::SingleThreaded);

    osgViewer::Viewer::Windows windows;
    getWindows(windows);

    _timerId = startTimer(10, Qt::PreciseTimer);
    _lastFrameStartTime.setStartTick(0);
}

void OSGRenderer::setKeyboardModifiers(QInputEvent* event)
{
    unsigned int modkey = event->modifiers() & (Qt::ShiftModifier |
                                                Qt::ControlModifier |
                                                Qt::AltModifier);
    unsigned int mask = 0;

    if(modkey & Qt::ShiftModifier) mask |= osgGA::GUIEventAdapter::MODKEY_SHIFT;

    if(modkey & Qt::ControlModifier) mask |= osgGA::GUIEventAdapter::MODKEY_CTRL;

    if(modkey & Qt::AltModifier) mask |= osgGA::GUIEventAdapter::MODKEY_ALT;

    m_osgWinEmb->getEventQueue()->getCurrentEventState()->setModKeyMask(mask);
}

void OSGRenderer::keyPressEvent(QKeyEvent* event)
{
    setKeyboardModifiers(event);
    int value = s_QtKeyboardMap.remapKey(event);
    m_osgWinEmb->getEventQueue()->keyPress(value);
}

void OSGRenderer::keyReleaseEvent(QKeyEvent* event)
{
    if(event->isAutoRepeat())
    {
        event->ignore();
    }
    else
    {
        setKeyboardModifiers(event);
        int value = s_QtKeyboardMap.remapKey(event);
        m_osgWinEmb->getEventQueue()->keyRelease(value);
    }
}

void OSGRenderer::mousePressEvent(QMouseEvent* event)
{
    int button = 0;

    switch(event->button())
    {
    case Qt::LeftButton:
        button = 1;
        break;

    case Qt::MidButton:
        button = 2;
        break;

    case Qt::RightButton:
        button = 3;
        break;

    case Qt::NoButton:
        button = 0;
        break;

    default:
        button = 0;
        break;
    }

    setKeyboardModifiers(event);
    m_osgWinEmb->getEventQueue()->mouseButtonPress(event->x() * m_windowScale,
                                                   event->y() * m_windowScale, button);
}

void OSGRenderer::mouseReleaseEvent(QMouseEvent* event)
{
    int button = 0;

    switch(event->button())
    {
    case Qt::LeftButton:
        button = 1;
        break;

    case Qt::MidButton:
        button = 2;
        break;

    case Qt::RightButton:
        button = 3;
        break;

    case Qt::NoButton:
        button = 0;
        break;

    default:
        button = 0;
        break;
    }

    setKeyboardModifiers(event);
    m_osgWinEmb->getEventQueue()->mouseButtonRelease(event->x() * m_windowScale,
                                                     event->y() * m_windowScale, button);
}

void OSGRenderer::mouseDoubleClickEvent(QMouseEvent* event)
{
    int button = 0;

    switch(event->button())
    {
    case Qt::LeftButton:
        button = 1;
        break;

    case Qt::MidButton:
        button = 2;
        break;

    case Qt::RightButton:
        button = 3;
        break;

    case Qt::NoButton:
        button = 0;
        break;

    default:
        button = 0;
        break;
    }

    setKeyboardModifiers(event);
    m_osgWinEmb->getEventQueue()->mouseDoubleButtonPress(event->x() * m_windowScale,
                                                         event->y() * m_windowScale, button);
}

void OSGRenderer::mouseMoveEvent(QMouseEvent* event)
{
    setKeyboardModifiers(event);
    m_osgWinEmb->getEventQueue()->mouseMotion(event->x() * m_windowScale,
                                              event->y() * m_windowScale);
}

void OSGRenderer::wheelEvent(QWheelEvent* event)
{
    setKeyboardModifiers(event);
    m_osgWinEmb->getEventQueue()->mouseMotion(event->x() * m_windowScale,
                                              event->y() * m_windowScale);
    m_osgWinEmb->getEventQueue()->mouseScroll(
        event->orientation() == Qt::Vertical ?
        (event->delta() > 0 ? osgGA::GUIEventAdapter::SCROLL_UP :
         osgGA::GUIEventAdapter::SCROLL_DOWN) :
        (event->delta() > 0 ? osgGA::GUIEventAdapter::SCROLL_LEFT :
         osgGA::GUIEventAdapter::SCROLL_RIGHT));
}

bool OSGRenderer::checkEvents()
{
    // check events from any attached sources
    for(Devices::iterator eitr = _eventSources.begin();
        eitr != _eventSources.end();
        ++eitr)
    {
        osgGA::Device* es = eitr->get();

        if(es->getCapabilities() & osgGA::Device::RECEIVE_EVENTS)
        {
            if(es->checkEvents())
                return true;
        }

    }

    // get events from all windows attached to Viewer.
    Windows windows;
    getWindows(windows);

    for(Windows::iterator witr = windows.begin();
        witr != windows.end();
        ++witr)
    {
        if((*witr)->checkEvents())
            return true;
    }

    return false;
}

bool OSGRenderer::checkNeedToDoFrame()
{
    // check if any event handler has prompted a redraw
    if(_requestRedraw)
        return true;

    if(_requestContinousUpdate)
        return true;

    // check if the view needs to update the scene graph
    // this check if camera has update callback and if scene requires to update scene graph
    if(requiresUpdateSceneGraph())
        return true;

    // check if the database pager needs to update the scene
    if(getDatabasePager()->requiresUpdateSceneGraph())
        return true;

    // check if the image pager needs to update the scene
    if(getImagePager()->requiresUpdateSceneGraph())
        return true;


    // check if the scene needs to be redrawn
    if(requiresRedraw())
        return true;

    // check if events are available and need processing
    if(checkEvents())
        return true;

    // and check again if any event handler has prompted a redraw
    if(_requestRedraw)
        return true;

    if(_requestContinousUpdate)
        return true;

    return false;
}

// called from ViewerWidget paintGL() method
void OSGRenderer::frame(double simulationTime)
{
    // limit the frame rate
    if(getRunMaxFrameRate() > 0.0)
    {
        double dt = _lastFrameStartTime.time_s();
        double minFrameTime = 1.0 / getRunMaxFrameRate();

        if(dt < minFrameTime)
            QThread::usleep(static_cast<unsigned int>(1000000.0 * (minFrameTime - dt)));
    }

    // avoid excessive CPU loading when no frame is required in ON_DEMAND mode
    if(getRunFrameScheme() == osgViewer::ViewerBase::ON_DEMAND)
    {
        double dt = _lastFrameStartTime.time_s();

        if(dt < 0.01)
            OpenThreads::Thread::microSleep(static_cast<unsigned int>(1000000.0 *
                                                                      (0.01 - dt)));
    }

    // record start frame time
    _lastFrameStartTime.setStartTick();
    // make frame

#if 1
    osgViewer::Viewer::frame(simulationTime);
#else

    if(_done) return;

    // OSG_NOTICE<<std::endl<<"CompositeViewer::frame()"<<std::endl<<std::endl;

    if(_firstFrame)
    {
        viewerInit();

        if(!isRealized())
        {
            realize();
        }

        _firstFrame = false;
    }

    advance(simulationTime);

    eventTraversal();
    updateTraversal();
    //    renderingTraversals();
#endif
}

void OSGRenderer::requestRedraw()
{
    osgViewer::Viewer::requestRedraw();
}

void OSGRenderer::timerEvent(QTimerEvent* /*event*/)
{
    // application is about to quit, just return
    if(_applicationAboutToQuit)
    {
        return;
    }

    // ask ViewerWidget to update 3D view
    if(getRunFrameScheme() != osgViewer::ViewerBase::ON_DEMAND ||
       checkNeedToDoFrame())
    {
        update();
    }
}
