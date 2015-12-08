
#include "pagebase.hpp"

#include <QLabel>

#include "../../model/prefs/category.hpp"

CSVPrefs::PageBase::PageBase (CSMPrefs::Category& category, QWidget *parent)
: QScrollArea (parent), mCategory (category)
{
QLabel *temp = new QLabel (category.getKey().c_str(), this);
setWidget (temp);

}

CSMPrefs::Category& CSVPrefs::PageBase::getCategory()
{
    return mCategory;
}
