#ifndef CSM_PREFS_SHORTCUTSETTING_H
#define CSM_PREFS_SHORTCUTSETTING_H

#include <QKeySequence>

#include "setting.hpp"

namespace CSMPrefs
{
    class ShortcutSetting : public Setting
    {
            Q_OBJECT

        public:

            typedef std::pair<QKeySequence, int> SequenceData;

            ShortcutSetting(Category* parent, Settings::Manager* values, QMutex* mutex, const std::string& key,
                const std::string& label, const SequenceData& default_);

            // TODO replace with custom page
            virtual std::pair<QWidget*, QWidget*> makeWidgets(QWidget* parent);

        private:

            SequenceData mDefault;

        private slots:

            void valueChanged(const QString& text);
    };
}

#endif
