#ifndef GRAPHICSPAGE_H
#define GRAPHICSPAGE_H

#include <QWidget>

#include <OgreRoot.h>
#include <OgreRenderSystem.h>
#include <OgreConfigFile.h>
#include <OgreConfigDialog.h>

// Static plugin headers
#ifdef ENABLE_PLUGIN_GL
# include "OgreGLPlugin.h"
#endif
#ifdef ENABLE_PLUGIN_Direct3D9
# include "OgreD3D9Plugin.h"
#endif

class QComboBox;
class QCheckBox;
class QStackedWidget;
class QSettings;

namespace Files { struct ConfigurationManager; }

class GraphicsPage : public QWidget
{
    Q_OBJECT

public:
    GraphicsPage(Files::ConfigurationManager &cfg, QWidget *parent = 0);

    void writeConfig();

public slots:
    void rendererChanged(const QString &renderer);

private:
    Ogre::Root *mOgre;
    Ogre::RenderSystem *mSelectedRenderSystem;
    Ogre::RenderSystem *mOpenGLRenderSystem;
    Ogre::RenderSystem *mDirect3DRenderSystem;
 	#ifdef ENABLE_PLUGIN_GL
 	Ogre::GLPlugin* mGLPlugin;
 	#endif
	#ifdef ENABLE_PLUGIN_Direct3D9
 	Ogre::D3D9Plugin* mD3D9Plugin;
 	#endif

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

    QSettings *mOgreConfig;

    Files::ConfigurationManager &mCfgMgr;

    QString getConfigValue(const QString &key, Ogre::RenderSystem *renderer);
    QStringList getAvailableOptions(const QString &key, Ogre::RenderSystem *renderer);

    void createPages();
    void setupConfig();
    void setupOgre();
    void readConfig();
};

#endif
