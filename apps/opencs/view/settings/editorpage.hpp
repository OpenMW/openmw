#ifndef EDITORPAGE_H
#define EDITORPAGE_H

#include "abstractpage.hpp"

class QGroupBox;

namespace CsSettings {

    class UserSettings;
    class AbstractBlock;

    class EditorPage : public AbstractPage
    {
        Q_OBJECT

    public:

        EditorPage(QWidget *parent = 0);

        void setupUi();
        void initializeWidgets (const SettingMap &settings);

    signals:
        void signalUpdateEditorSetting (const QString &settingName, const QString &settingValue);
    };
}
#endif //EDITORPAGE_H
