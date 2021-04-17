#ifndef CSM_PREFS_StringSetting_H
#define CSM_PREFS_StringSetting_H

#include "setting.hpp"

class QLineEdit;

namespace CSMPrefs
{
    class StringSetting : public Setting
    {
            Q_OBJECT

            std::string mTooltip;
            std::string mDefault;
            QLineEdit* mWidget;

        public:

            StringSetting (Category *parent, Settings::Manager *values,
                QMutex *mutex, const std::string& key, const std::string& label, std::string default_);

            StringSetting& setTooltip (const std::string& tooltip);

            /// Return label, input widget.
            std::pair<QWidget *, QWidget *> makeWidgets (QWidget *parent) override;

            void updateWidget() override;

        private slots:

            void textChanged (const QString& text);
    };
}

#endif
