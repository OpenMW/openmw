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
    class StringSetting : public Setting
    {
        Q_OBJECT

        std::string mTooltip;
        std::string mDefault;
        QLineEdit* mWidget;

    public:
        StringSetting(
            Category* parent, QMutex* mutex, const std::string& key, const std::string& label, std::string default_);

        StringSetting& setTooltip(const std::string& tooltip);

        /// Return label, input widget.
        std::pair<QWidget*, QWidget*> makeWidgets(QWidget* parent) override;

        void updateWidget() override;

    private slots:

        void textChanged(const QString& text);
    };
}

#endif
