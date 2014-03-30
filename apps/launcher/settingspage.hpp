#ifndef SETTINGSPAGE_HPP
#define SETTINGSPAGE_HPP

#include <QWidget>

#include "ui_settingspage.h"

namespace Launcher
{

    class SettingsPage : public QWidget, private Ui::SettingsPage
    {
        Q_OBJECT

    public:
        SettingsPage(QWidget *parent = 0);

        void saveSettings();
        bool loadSettings();

    private slots:
        void on_wizardButton_clicked();
        void on_importerButton_clicked();
    };
}

#endif // SETTINGSPAGE_HPP
