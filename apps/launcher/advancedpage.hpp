#ifndef ADVANCEDPAGE_H
#define ADVANCEDPAGE_H

#include <QWidget>

#include "ui_advancedpage.h"

#include <components/settings/settings.hpp>

namespace Files { struct ConfigurationManager; }
namespace Config { class GameSettings; }

namespace Launcher
{
    class AdvancedPage : public QWidget, private Ui::AdvancedPage
    {
        Q_OBJECT

    public:
        AdvancedPage(Files::ConfigurationManager &cfg, Config::GameSettings &gameSettings,
                     Settings::Manager &engineSettings, QWidget *parent = 0);

        bool loadSettings();
        void saveSettings();

    public slots:
        void slotLoadedCellsChanged(QStringList cellNames);

    private slots:
        void on_skipMenuCheckBox_stateChanged(int state);
        void on_runScriptAfterStartupBrowseButton_clicked();
        void slotAnimSourcesToggled(bool checked);

    private:
        Files::ConfigurationManager &mCfgMgr;
        Config::GameSettings &mGameSettings;
        Settings::Manager &mEngineSettings;

        /**
         * Load the cells associated with the given content files for use in autocomplete
         * @param filePaths the file paths of the content files to be examined
         */
        void loadCellsForAutocomplete(QStringList filePaths);
        void loadSettingBool(QCheckBox *checkbox, const std::string& setting, const std::string& group);
        void saveSettingBool(QCheckBox *checkbox, const std::string& setting, const std::string& group);
    };
}
#endif
