#ifndef GRAPHICSPAGE_H
#define GRAPHICSPAGE_H

#include <QWidget>

#include "ui_graphicspage.h"

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
        void screenChanged(int screen);

    private slots:
        void slotFullScreenChanged(int state);
        void slotStandardToggled(bool checked);

    private:
        Files::ConfigurationManager &mCfgMgr;
        GraphicsSettings &mGraphicsSettings;

        QStringList getAvailableResolutions(int screen);
        QRect getMaximumResolution();

        bool setupSDL();
    };
}
#endif
