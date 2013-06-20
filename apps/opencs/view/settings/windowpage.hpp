#ifndef WINDOWPAGE_H
#define WINDOWPAGE_H

#include "abstractpage.hpp"

class QGroupBox;

namespace CSVSettings {

    class UserSettings;
    class AbstractBlock;

    class WindowPage : public AbstractPage
    {
        Q_OBJECT

    public:

        WindowPage(QWidget *parent = 0);

        void setupUi();
        void initializeWidgets (const CSMSettings::SettingMap &settings);

        ///
        GroupBlockDef *buildCustomWindowSize();
        GroupBlockDef *buildDefinedWindowSize();
        GroupBlockDef *buildWindowSizeToggle();
        CustomBlockDef *buildWindowSize (GroupBlockDef *, GroupBlockDef *, GroupBlockDef *);

    signals:
        void signalUpdateEditorSetting (const QString &settingName, const QString &settingValue);
    };
}
#endif //WINDOWPAGE_H
