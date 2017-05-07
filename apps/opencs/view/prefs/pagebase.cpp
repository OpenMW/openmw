
#include "pagebase.hpp"

#include "../../model/prefs/category.hpp"
#include "../../model/prefs/state.hpp"

CSVPrefs::PageBase::PageBase (CSMPrefs::Category& category, QWidget *parent)
: QScrollArea (parent), mCategory (category)
{}

CSMPrefs::Category& CSVPrefs::PageBase::getCategory()
{
    return mCategory;
}

void CSVPrefs::PageBase::resetCategory()
{
    CSMPrefs::get().resetCategory(getCategory().getKey());
    refresh();
}
