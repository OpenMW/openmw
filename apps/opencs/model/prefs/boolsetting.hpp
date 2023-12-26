#ifndef CSM_PREFS_BOOLSETTING_H
#define CSM_PREFS_BOOLSETTING_H

#include "setting.hpp"

#include <string>
#include <utility>

class QCheckBox;

namespace CSMPrefs
{
    class Category;

    class BoolSetting final : public TypedSetting<bool>
    {
        Q_OBJECT

        std::string mTooltip;
        QCheckBox* mWidget;

    public:
        explicit BoolSetting(
            Category* parent, QMutex* mutex, std::string_view key, const QString& label, Settings::Index& index);

        BoolSetting& setTooltip(const std::string& tooltip);

        /// Return label, input widget.
        SettingWidgets makeWidgets(QWidget* parent) override;

        void updateWidget() override;

    private slots:

        void valueChanged(int value);
    };
}

#endif
