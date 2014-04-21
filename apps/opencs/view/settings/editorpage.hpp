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

        void initializeWidgets (const CSMSettings::DefinitionMap &settings);
        void setupUi();

    private:

        /// User preference view of the record status delegate's icon / text setting
        GroupBlockDef *setupRecordStatusDisplay();

    signals:

        /// Signals up for changes to editor application-level settings
        void signalUpdateEditorSetting (const QString &settingName, const QString &settingValue);

    public slots:
    };
}

#endif // EDITORPAGE_HPP
