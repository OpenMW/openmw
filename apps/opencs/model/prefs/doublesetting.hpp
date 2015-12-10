#ifndef CSM_PREFS_DOUBLESETTING_H
#define CSM_PREFS_DOUBLESETTING_H

#include "setting.hpp"

namespace CSMPrefs
{
    class DoubleSetting : public Setting
    {
            Q_OBJECT

            double mMin;
            double mMax;
            std::string mTooltip;
            double mDefault;

        public:

            DoubleSetting (Category *parent, Settings::Manager *values,
                const std::string& key, const std::string& label, double default_);

            DoubleSetting& setRange (double min, double max);

            DoubleSetting& setMin (double min);

            DoubleSetting& setMax (double max);

            DoubleSetting& setTooltip (const std::string& tooltip);

            /// Return label, input widget.
            virtual std::pair<QWidget *, QWidget *> makeWidgets (QWidget *parent);

        private slots:

            void valueChanged (double value);
    };
}

#endif
