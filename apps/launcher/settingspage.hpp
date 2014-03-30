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

    };
}

#endif // SETTINGSPAGE_HPP
