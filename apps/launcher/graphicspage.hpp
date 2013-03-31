#ifndef GRAPHICSPAGE_H
#define GRAPHICSPAGE_H

#include <QWidget>

#include <OgreRoot.h>
#include <OgreRenderSystem.h>
//#include <OgreConfigFile.h>
//#include <OgreConfigDialog.h>

// Static plugin headers
#ifdef ENABLE_PLUGIN_GL
# include "OgreGLPlugin.h"
#endif
#ifdef ENABLE_PLUGIN_Direct3D9
# include "OgreD3D9Plugin.h"
#endif

#include "ui_graphicspage.h"

class GraphicsSettings;

namespace Files { struct ConfigurationManager; }

class GraphicsPage : public QWidget, private Ui::GraphicsPage
{
    Q_OBJECT

public:
    GraphicsPage(Files::ConfigurationManager &cfg, GraphicsSettings &graphicsSettings, QWidget *parent = 0);

    void saveSettings();
    bool setupOgre();

public slots:
    void rendererChanged(const QString &renderer);

private slots:
    void slotFullScreenChanged(int state);
    void slotStandardToggled(bool checked);

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

    Files::ConfigurationManager &mCfgMgr;
    GraphicsSettings &mGraphicsSettings;

    QStringList getAvailableOptions(const QString &key, Ogre::RenderSystem *renderer);
    QStringList getAvailableResolutions(Ogre::RenderSystem *renderer);

    void loadSettings();

};

#endif
