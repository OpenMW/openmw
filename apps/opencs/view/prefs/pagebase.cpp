
#include "pagebase.hpp"

#include "../../model/prefs/category.hpp"

CSVPrefs::PageBase::PageBase (CSMPrefs::Category& category, QWidget *parent)
: QScrollArea (parent), mCategory (category)
{}

CSMPrefs::Category& CSVPrefs::PageBase::getCategory()
{
    return mCategory;
}
