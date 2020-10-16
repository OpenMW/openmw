#ifndef CSM_PREFS_INTSETTING_H
#define CSM_PREFS_INTSETTING_H

#include "setting.hpp"

class QSpinBox;

namespace CSMPrefs
{
    class IntSetting : public Setting
    {
            Q_OBJECT

            int mMin;
            int mMax;
            std::string mTooltip;
            int mDefault;
            QSpinBox* mWidget;

        public:

            IntSetting (Category *parent, Settings::Manager *values,
                QMutex *mutex, const std::string& key, const std::string& label, int default_);

            // defaults to [0, std::numeric_limits<int>::max()]
            IntSetting& setRange (int min, int max);

            IntSetting& setMin (int min);

            IntSetting& setMax (int max);

            IntSetting& setTooltip (const std::string& tooltip);

            /// Return label, input widget.
            std::pair<QWidget *, QWidget *> makeWidgets (QWidget *parent) override;

            void updateWidget() override;

        private slots:

            void valueChanged (int value);
    };
}

#endif
