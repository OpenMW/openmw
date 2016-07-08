#ifndef CSM_PREFS_SHORTCUTSETTING_H
#define CSM_PREFS_SHORTCUTSETTING_H

#include <QKeySequence>

#include "setting.hpp"

namespace CSMPrefs
{
    class ShortcutSetting : public Setting
    {
            Q_OBJECT

            QKeySequence mDefault;

        public:

            ShortcutSetting(Category* parent, Settings::Manager* values, QMutex* mutex, const std::string& key,
                const std::string& label, const QKeySequence& default_);

            // TODO replace with custom page
            virtual std::pair<QWidget*, QWidget*> makeWidgets(QWidget* parent);

        private slots:

            void valueChanged(const QString& text);
    };
}

#endif
