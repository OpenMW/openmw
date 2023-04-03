#ifndef COMPOSITEOSGRENDERER_H
#define COMPOSITEOSGRENDERER_H

#include <QObject>
#include <QTimer>

#include <osgViewer/CompositeViewer>

class CompositeOsgRenderer : public QObject, public osgViewer::CompositeViewer
{
    bool m_osgInitialized {false};
    osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> m_osgWinEmb;

    osg::Timer mFrameTimer;
    double mSimulationTime;
    Q_OBJECT

public:

    explicit CompositeOsgRenderer(QObject* parent = nullptr);
    explicit CompositeOsgRenderer(osg::ArgumentParser* arguments, QObject* parent = nullptr);

    ~CompositeOsgRenderer() override;

    virtual void resize(int windowWidth, int windowHeight);

    void setGraphicsWindowEmbedded(osg::ref_ptr<osgViewer::GraphicsWindowEmbedded> osgWinEmb);

    void setupOSG();

    // overrided from osgViewer::ViewerBase
    void frame(double simulationTime = USE_REFERENCE_TIME) override;

    QTimer mTimer;

protected:
    void timerEvent(QTimerEvent* event) override;
  
public slots:
    void update();

signals:
    void simulationUpdated(double dt);
};

#endif // COMPOSITEOSGRENDERER_H
