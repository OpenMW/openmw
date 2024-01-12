#ifndef OPENMW_APPS_OPENCS_MODEL_PREFS_SUBCATEGORY_H
#define OPENMW_APPS_OPENCS_MODEL_PREFS_SUBCATEGORY_H

#include "setting.hpp"

#include <string>
#include <utility>

namespace CSMPrefs
{
    class Category;

    class Subcategory final : public Setting
    {
        Q_OBJECT

    public:
        explicit Subcategory(Category* parent, QMutex* mutex, const QString& label, Settings::Index& index);

        SettingWidgets makeWidgets(QWidget* parent) override;

        void updateWidget() override {}

        void reset() override {}
    };
}

#endif
