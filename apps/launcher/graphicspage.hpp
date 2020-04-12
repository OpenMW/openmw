#ifndef GRAPHICSPAGE_H
#define GRAPHICSPAGE_H

#include <QWidget>

#include "ui_graphicspage.h"

#include <components/settings/settings.hpp>

#include "sdlinit.hpp"

namespace Files { struct ConfigurationManager; }

namespace Launcher
{
    class GraphicsSettings;

    class GraphicsPage : public QWidget, private Ui::GraphicsPage
    {
        Q_OBJECT

    public:
        GraphicsPage(Files::ConfigurationManager &cfg, Settings::Manager &engineSettings, QWidget *parent = 0);

        void saveSettings();
        bool loadSettings();

    public slots:
        void screenChanged(int screen);

    private slots:
        void slotFullScreenChanged(int state);
        void slotStandardToggled(bool checked);
        void slotFramerateLimitToggled(bool checked);
        void slotShadowDistLimitToggled(bool checked);

    private:
        Files::ConfigurationManager &mCfgMgr;
        Settings::Manager &mEngineSettings;

        QStringList getAvailableResolutions(int screen);
        QRect getMaximumResolution();

        bool setupSDL();
    };
}
#endif
