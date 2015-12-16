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
                QMutex *mutex, const std::string& key, const std::string& label,
                double default_);

            // defaults to [0, std::numeric_limits<double>::max()]
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
