#ifndef CSV_PREFS_CONTEXTMENULIST_H
#define CSV_PREFS_CONTEXTMENULIST_H

#include <QListWidget>

class QContextMenuEvent;

namespace CSVPrefs
{
    class ContextMenuList : public QListWidget
    {
            Q_OBJECT
    
        public:
    
            ContextMenuList(QWidget* parent = 0);
    
        protected:
    
            void contextMenuEvent(QContextMenuEvent* e);
    
        private slots:
    
            void resetCategory();
    
            void resetAll();
        };
}

#endif
