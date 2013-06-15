#ifndef SAMPLEPAGE_H
#define SAMPLEPAGE_H

#include "abstractpage.hpp"

class QGroupBox;

namespace CSVSettings {

    class UserSettings;
    class AbstractBlock;

    class SamplePage : public AbstractPage
    {
        Q_OBJECT

    public:

        explicit SamplePage(QWidget *parent = 0);

        void setupUi();
        void initializeWidgets (const CSMSettings::SettingMap &settings);

    signals:
        void signalUpdateEditorSetting (const QString &settingName, const QString &settingValue);
    };
}
#endif //SAMPLEPAGE_H
