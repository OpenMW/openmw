#ifndef CSM_PREFS_ENUMSETTING_H
#define CSM_PREFS_ENUMSETTING_H

#include <vector>

#include "setting.hpp"

class QComboBox;

namespace CSMPrefs
{
    struct EnumValue
    {
        std::string mValue;
        std::string mTooltip;

        EnumValue (const std::string& value, const std::string& tooltip = "");

        EnumValue (const char *value);
    };

    struct EnumValues
    {
        std::vector<EnumValue> mValues;

        EnumValues& add (const EnumValues& values);

        EnumValues& add (const EnumValue& value);

        EnumValues& add (const std::string& value, const std::string& tooltip);
    };

    class EnumSetting : public Setting
    {
            Q_OBJECT

            std::string mTooltip;
            EnumValue mDefault;
            EnumValues mValues;
            QComboBox* mWidget;

        public:

            EnumSetting (Category *parent, Settings::Manager *values,
                QMutex *mutex, const std::string& key, const std::string& label,
                const EnumValue& default_);

            EnumSetting& setTooltip (const std::string& tooltip);

            EnumSetting& addValues (const EnumValues& values);

            EnumSetting& addValue (const EnumValue& value);

            EnumSetting& addValue (const std::string& value, const std::string& tooltip);

            /// Return label, input widget.
            std::pair<QWidget *, QWidget *> makeWidgets (QWidget *parent) override;

            void updateWidget() override;

        private slots:

            void valueChanged (int value);
    };
}

#endif
