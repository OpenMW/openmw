#ifndef CSV_PREFS_PAGEBASE_H
#define CSV_PREFS_PAGEBASE_H

#include <QScrollArea>

class QContextMenuEvent;

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

        protected:

            void contextMenuEvent(QContextMenuEvent*) override;

        private slots:

            void resetCategory();

            void resetAll();
    };
}

#endif
