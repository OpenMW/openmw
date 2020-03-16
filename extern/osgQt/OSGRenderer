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

#ifndef OSGRENDERER_H
#define OSGRENDERER_H

#include <QObject>

#include <osgViewer/Viewer>

class QInputEvent;
class QKeyEvent;
class QMouseEvent;
class QWheelEvent;
namespace eveBIM
{
    class ViewerWidget;
}

class OSGRenderer : public QObject, public osgViewer::Viewer
{
    bool                                       m_osgInitialized {false};
    osg::ref_ptr<osgViewer::GraphicsWindow>    m_osgWinEmb;
    float                                      m_windowScale {1.0f};
    bool                                       m_continuousUpdate {true};

    int                                        _timerId{0};
    osg::Timer                                 _lastFrameStartTime;
    bool                                       _applicationAboutToQuit {false};
    bool                                       _osgWantsToRenderFrame{true};

    Q_OBJECT

    friend class eveBIM::ViewerWidget;

public:

    explicit OSGRenderer(QObject* parent = nullptr);
    explicit OSGRenderer(osg::ArgumentParser* arguments, QObject* parent = nullptr);

    ~OSGRenderer() override;

    bool continuousUpdate() const
    {
        return m_continuousUpdate;
    }
    void setContinuousUpdate(bool continuousUpdate)
    {
        m_continuousUpdate = continuousUpdate;
    }

    virtual void keyPressEvent(QKeyEvent* event);
    virtual void keyReleaseEvent(QKeyEvent* event);
    virtual void mousePressEvent(QMouseEvent* event);
    virtual void mouseReleaseEvent(QMouseEvent* event);
    virtual void mouseDoubleClickEvent(QMouseEvent* event);
    virtual void mouseMoveEvent(QMouseEvent* event);
    virtual void wheelEvent(QWheelEvent* event);

    virtual void resize(int windowWidth, int windowHeight, float windowScale);

    void setupOSG(int windowWidth, int windowHeight, float windowScale);

    // overrided from osgViewer::Viewer
    virtual bool checkNeedToDoFrame() override;

    // overrided from osgViewer::ViewerBase
    void frame(double simulationTime = USE_REFERENCE_TIME) override;

    // overrided from osgViewer::Viewer
    void requestRedraw() override;
    // overrided from osgViewer::Viewer
    bool checkEvents() override;
    void update();

protected:
    void timerEvent(QTimerEvent* event) override;

    void setKeyboardModifiers(QInputEvent* event);

};

#endif // OSGRENDERER_H
