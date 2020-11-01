#ifndef CSM_PREFS_BOOLSETTING_H
#define CSM_PREFS_BOOLSETTING_H

#include "setting.hpp"

class QCheckBox;

namespace CSMPrefs
{
    class BoolSetting : public Setting
    {
            Q_OBJECT

            std::string mTooltip;
            bool mDefault;
            QCheckBox* mWidget;

        public:

            BoolSetting (Category *parent, Settings::Manager *values,
                QMutex *mutex, const std::string& key, const std::string& label, bool default_);

            BoolSetting& setTooltip (const std::string& tooltip);

            /// Return label, input widget.
            std::pair<QWidget *, QWidget *> makeWidgets (QWidget *parent) override;

            void updateWidget() override;

        private slots:

            void valueChanged (int value);
    };
}

#endif
