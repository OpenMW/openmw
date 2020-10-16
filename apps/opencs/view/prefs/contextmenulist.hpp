#ifndef CSV_PREFS_CONTEXTMENULIST_H
#define CSV_PREFS_CONTEXTMENULIST_H

#include <QListWidget>

class QContextMenuEvent;
class QMouseEvent;

namespace CSVPrefs
{
    class ContextMenuList : public QListWidget
    {
            Q_OBJECT
    
        public:
    
            ContextMenuList(QWidget* parent = nullptr);
    
        protected:
    
            void contextMenuEvent(QContextMenuEvent* e) override;

            void mousePressEvent(QMouseEvent* e) override;
    
        private slots:
    
            void resetCategory();
    
            void resetAll();
        };
}

#endif
