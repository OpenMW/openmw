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
                const std::string& label);

            virtual std::pair<QWidget*, QWidget*> makeWidgets(QWidget* parent);

            virtual void updateWidget();

        protected:

            bool eventFilter(QObject* target, QEvent* event);

        private:

            bool handleEvent(QObject* target, int mod, int value, bool active);

            void storeValue(const QKeySequence& sequence);
            void resetState();

            static const int MaxKeys = 4;

            QPushButton* mButton;

            bool mEditorActive;
            int mEditorPos;
            int mEditorKeys[MaxKeys];

        private slots:

            void buttonToggled(bool checked);
    };
}

#endif
