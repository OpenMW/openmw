#ifndef EDITORPAGE_HPP
#define EDITORPAGE_HPP

#include "support.hpp"
#include "abstractpage.hpp"

namespace CSVSettings
{
    class EditorPage : public AbstractPage
    {
        Q_OBJECT

    public:
        explicit EditorPage(QWidget *parent = 0);
        explicit EditorPage (const QString &pageName, QWidget* parent = 0);

        void initializeWidgets (const CSMSettings::SettingMap &settings);
        void setupUi();

    private:
        GroupBlockDef *setupRecordStatusDisplay();

    signals:
        void signalUpdateEditorSetting (const QString &settingName, const QString &settingValue);

    public slots:
    };
}

#endif // EDITORPAGE_HPP
