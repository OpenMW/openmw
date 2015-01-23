#ifndef GRAPHICSPAGE_H
#define GRAPHICSPAGE_H

#include <QWidget>

#include <components/ogreinit/ogreinit.hpp>

#include "ui_graphicspage.h"

namespace Ogre { class Root; class RenderSystem; }

namespace Files { struct ConfigurationManager; }

namespace Launcher
{
    class GraphicsSettings;

    class GraphicsPage : public QWidget, private Ui::GraphicsPage
    {
        Q_OBJECT

    public:
        GraphicsPage(Files::ConfigurationManager &cfg, GraphicsSettings &graphicsSettings, QWidget *parent = 0);

        void saveSettings();
        bool loadSettings();

    public slots:
        void rendererChanged(const QString &renderer);
        void screenChanged(int screen);

    private slots:
        void slotFullScreenChanged(int state);
        void slotStandardToggled(bool checked);

    private:
        OgreInit::OgreInit mOgreInit;
        Ogre::Root *mOgre;
        Ogre::RenderSystem *mSelectedRenderSystem;
        Ogre::RenderSystem *mOpenGLRenderSystem;
        Ogre::RenderSystem *mDirect3DRenderSystem;

        Files::ConfigurationManager &mCfgMgr;
        GraphicsSettings &mGraphicsSettings;

        QStringList getAvailableOptions(const QString &key, Ogre::RenderSystem *renderer);
        QStringList getAvailableResolutions(int screen);
        QRect getMaximumResolution();

        bool setupOgre();
        bool setupSDL();
    };
}
#endif
