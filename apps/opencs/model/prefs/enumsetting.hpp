#ifndef CSM_PREFS_ENUMSETTING_H
#define CSM_PREFS_ENUMSETTING_H

#include <span>
#include <string>
#include <string_view>
#include <utility>
#include <vector>

#include "enumvalueview.hpp"
#include "setting.hpp"

class QComboBox;

namespace CSMPrefs
{
    class Category;

    class EnumSetting final : public TypedSetting<std::string>
    {
        Q_OBJECT

        std::string mTooltip;
        std::span<const EnumValueView> mValues;
        QComboBox* mWidget;

    public:
        explicit EnumSetting(Category* parent, QMutex* mutex, std::string_view key, const QString& label,
            std::span<const EnumValueView> values, Settings::Index& index);

        EnumSetting& setTooltip(const std::string& tooltip);

        /// Return label, input widget.
        SettingWidgets makeWidgets(QWidget* parent) override;

        void updateWidget() override;

    private slots:

        void valueChanged(int value);
    };
}

#endif
