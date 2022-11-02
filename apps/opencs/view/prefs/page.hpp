#ifndef CSV_PREFS_PAGE_H
#define CSV_PREFS_PAGE_H

#include "pagebase.hpp"

class QGridLayout;
class QWidget;
class QObject;

namespace CSMPrefs
{
    class Category;
    class Setting;
}

namespace CSVPrefs
{
    class Page : public PageBase
    {
        Q_OBJECT

        QGridLayout* mGrid;

    public:
        Page(CSMPrefs::Category& category, QWidget* parent);

        void addSetting(CSMPrefs::Setting* setting);
    };
}

#endif
