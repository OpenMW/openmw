#ifndef GRAPHICSPAGE_H
#define GRAPHICSPAGE_H

#include "ui_graphicspage.h"

#include <components/settings/windowmode.hpp>

namespace Files
{
    struct ConfigurationManager;
}

namespace Launcher
{
    class GraphicsSettings;

    class GraphicsPage : public QWidget, private Ui::GraphicsPage
    {
        Q_OBJECT

    public:
        explicit GraphicsPage(QWidget* parent = nullptr);

        void saveSettings();
        bool loadSettings();

    public slots:
        void screenChanged(int screen);

    private slots:
        void slotFullScreenChanged(int state);
        void slotStandardToggled(bool checked);
        void slotFramerateLimitToggled(bool checked);

    private:
        QVector<QStringList> mResolutionsPerScreen;

        static QStringList getAvailableResolutions(int screen);
        static QRect getMaximumResolution();

        bool setupSDL();
        void handleWindowModeChange(Settings::WindowMode state);
    };
}
#endif
