#ifndef CSV_PREFS_CONTEXTMENUWIDGET_H
#define CSV_PREFS_CONTEXTMENUWIDGET_H

#include <string>

#include <QWidget>

class QContextMenuEvent;

namespace CSVPrefs
{
    class ContextMenuWidget : public QWidget
    {
            Q_OBJECT
    
        public:
    
            ContextMenuWidget(QWidget* parent, const std::string& category);
    
        protected:
    
            void contextMenuEvent(QContextMenuEvent* e);
    
        private slots:
    
            void resetCategory();
    
            void resetAll();

        private:

            std::string mCategory;
        };
}

#endif
