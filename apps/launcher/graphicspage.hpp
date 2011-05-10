#ifndef GRAPHICSPAGE_H
#define GRAPHICSPAGE_H

#include <QWidget>

#include <OgreRoot.h>
#include <OgreRenderSystem.h>
#include <OgreConfigFile.h>
#include <OgreConfigDialog.h>

class QComboBox;
class QCheckBox;
class QStackedWidget;
class QSettings;

class GraphicsPage : public QWidget
{
    Q_OBJECT

public:
    GraphicsPage(QWidget *parent = 0);

    QSettings *mOgreConfig;

    void writeConfig();

public slots:
    void rendererChanged(const QString &renderer);

private:
    Ogre::Root *mOgre;
    Ogre::RenderSystem *mSelectedRenderSystem;
    Ogre::RenderSystem *mOpenGLRenderSystem;
    Ogre::RenderSystem *mDirect3DRenderSystem;

    QComboBox *mRendererComboBox;

    QStackedWidget *mRendererStackedWidget;
    QStackedWidget *mDisplayStackedWidget;

    // OpenGL
    QComboBox *mOGLRTTComboBox;
    QComboBox *mOGLAntiAliasingComboBox;
    QComboBox *mOGLResolutionComboBox;
    QComboBox *mOGLFrequencyComboBox;

    QCheckBox *mOGLVSyncCheckBox;
    QCheckBox *mOGLFullScreenCheckBox;

    // Direct3D
    QComboBox *mD3DRenderDeviceComboBox;
    QComboBox *mD3DAntiAliasingComboBox;
    QComboBox *mD3DFloatingPointComboBox;
    QComboBox *mD3DResolutionComboBox;

    QCheckBox *mD3DNvPerfCheckBox;
    QCheckBox *mD3DVSyncCheckBox;
    QCheckBox *mD3DFullScreenCheckBox;

    QString getConfigValue(const QString &key, Ogre::RenderSystem *renderer);
    QStringList getAvailableOptions(const QString &key, Ogre::RenderSystem *renderer);

    void createPages();
    void setupConfig();
    void setupOgre();
    void readConfig();
};

#endif
