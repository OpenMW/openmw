#ifndef CSM_PREFS_ENUMSETTING_H
#define CSM_PREFS_ENUMSETTING_H

#include <string>
#include <utility>
#include <vector>

#include "setting.hpp"

class QComboBox;

namespace CSMPrefs
{
    class Category;

    struct EnumValue
    {
        std::string mValue;
        std::string mTooltip;

        EnumValue(const std::string& value, const std::string& tooltip = "");

        EnumValue(const char* value);
    };

    struct EnumValues
    {
        std::vector<EnumValue> mValues;

        EnumValues& add(const EnumValues& values);

        EnumValues& add(const EnumValue& value);

        EnumValues& add(const std::string& value, const std::string& tooltip);
    };

    class EnumSetting final : public TypedSetting<std::string>
    {
        Q_OBJECT

        std::string mTooltip;
        EnumValues mValues;
        QComboBox* mWidget;

    public:
        explicit EnumSetting(
            Category* parent, QMutex* mutex, const std::string& key, const QString& label, Settings::Index& index);

        EnumSetting& setTooltip(const std::string& tooltip);

        EnumSetting& addValues(const EnumValues& values);

        EnumSetting& addValue(const EnumValue& value);

        EnumSetting& addValue(const std::string& value, const std::string& tooltip);

        /// Return label, input widget.
        SettingWidgets makeWidgets(QWidget* parent) override;

        void updateWidget() override;

    private slots:

        void valueChanged(int value);
    };
}

#endif
