#ifndef CSV_PREFS_PAGEBASE_H
#define CSV_PREFS_PAGEBASE_H

#include <QScrollArea>

namespace CSMPrefs
{
    class Category;
}

namespace CSVPrefs
{
    class PageBase : public QScrollArea
    {
            Q_OBJECT

            CSMPrefs::Category& mCategory;

        public:

            PageBase (CSMPrefs::Category& category, QWidget *parent);

            CSMPrefs::Category& getCategory();
    };
}

#endif
