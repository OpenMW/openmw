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
        QGridLayout* const layout = new QGridLayout();
        layout->setSizeConstraint(QLayout::SetMinAndMaxSize);

        return SettingWidgets{ .mLabel = nullptr, .mInput = nullptr, .mLayout = layout };
    }
}
