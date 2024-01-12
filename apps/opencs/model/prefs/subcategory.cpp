#include "subcategory.hpp"

#include <QGridLayout>

namespace CSMPrefs
{
    class Category;

    Subcategory::Subcategory(Category* parent, QMutex* mutex, const QString& label, Settings::Index& index)
        : Setting(parent, mutex, "", label, index)
    {
    }

    SettingWidgets Subcategory::makeWidgets(QWidget* /*parent*/)
    {
        return SettingWidgets{ .mLabel = nullptr, .mInput = nullptr };
    }
}
