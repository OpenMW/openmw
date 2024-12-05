#ifndef CSM_PREFS_INTSETTING_H
#define CSM_PREFS_INTSETTING_H

#include "setting.hpp"

class QSpinBox;
class QMutex;
class QObject;
class QWidget;

namespace CSMPrefs
{
    class Category;

    class IntSetting final : public TypedSetting<int>
    {
        Q_OBJECT

        int mMin;
        int mMax;
        std::string mTooltip;
        QSpinBox* mWidget;

    public:
        explicit IntSetting(
            Category* parent, QMutex* mutex, std::string_view key, const QString& label, Settings::Index& index);

        // defaults to [0, std::numeric_limits<int>::max()]
        IntSetting& setRange(int min, int max);

        IntSetting& setMin(int min);

        IntSetting& setMax(int max);

        IntSetting& setTooltip(const std::string& tooltip);

        /// Return label, input widget.
        SettingWidgets makeWidgets(QWidget* parent) override;

        void updateWidget() override;

    private slots:

        void valueChanged(int value);
    };
}

#endif
