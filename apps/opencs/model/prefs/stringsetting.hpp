#ifndef CSM_PREFS_StringSetting_H
#define CSM_PREFS_StringSetting_H

#include "setting.hpp"

#include <string>
#include <utility>

class QLineEdit;
class QMutex;
class QObject;
class QWidget;

namespace CSMPrefs
{
    class Category;

    class StringSetting final : public TypedSetting<std::string>
    {
        Q_OBJECT

        std::string mTooltip;
        QLineEdit* mWidget;

    public:
        explicit StringSetting(
            Category* parent, QMutex* mutex, std::string_view key, const QString& label, Settings::Index& index);

        StringSetting& setTooltip(const std::string& tooltip);

        /// Return label, input widget.
        SettingWidgets makeWidgets(QWidget* parent) override;

        void updateWidget() override;

    private slots:

        void textChanged(const QString& text);
    };
}

#endif
