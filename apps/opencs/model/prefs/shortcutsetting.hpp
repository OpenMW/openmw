#ifndef CSM_PREFS_SHORTCUTSETTING_H
#define CSM_PREFS_SHORTCUTSETTING_H

#include <QKeySequence>

#include "setting.hpp"

class QEvent;
class QPushButton;

namespace CSMPrefs
{
    class ShortcutSetting : public Setting
    {
            Q_OBJECT

        public:

            ShortcutSetting(Category* parent, Settings::Manager* values, QMutex* mutex, const std::string& key,
                const std::string& label, const QKeySequence& default_);

            virtual std::pair<QWidget*, QWidget*> makeWidgets(QWidget* parent);

        protected:

            bool eventFilter(QObject* target, QEvent* event);

        private:

            bool handleEvent(QObject* target, int mod, int value, bool active);

            QKeySequence mDefault;

            QPushButton* mButton;

            bool mEditorActive;
            int mEditorPos;
            int mEditorKeys[4];

        private slots:

            void buttonToggled(bool checked);
    };
}

#endif
