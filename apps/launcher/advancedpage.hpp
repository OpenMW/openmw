#ifndef ADVANCEDPAGE_H
#define ADVANCEDPAGE_H

#include <QWidget>

#include "ui_advancedpage.h"

#include <components/settings/settings.hpp>

namespace Files { struct ConfigurationManager; }

namespace Launcher
{
    class AdvancedPage : public QWidget, private Ui::AdvancedPage
    {
        Q_OBJECT

    public:
        AdvancedPage(Files::ConfigurationManager &cfg, Settings::Manager &engineSettings, QWidget *parent = 0);

        bool loadSettings();
        void saveSettings();

    private:
        Files::ConfigurationManager &mCfgMgr;
        Settings::Manager &mEngineSettings;

        void loadSettingBool(QCheckBox *checkbox, const std::string& setting, const std::string& group);
        void saveSettingBool(QCheckBox *checkbox, const std::string& setting, const std::string& group);
    };
}
#endif
