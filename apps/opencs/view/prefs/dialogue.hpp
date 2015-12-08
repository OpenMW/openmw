#ifndef CSV_PREFS_DIALOGUE_H
#define CSV_PREFS_DIALOGUE_H

#include <QMainWindow>

class QSplitter;
class QListWidget;
class QStackedWidget;

namespace CSVPrefs
{
    class Dialogue : public QMainWindow
    {
            Q_OBJECT

            QListWidget *mList;
            QStackedWidget *mContent;

        private:

            void buildCategorySelector (QSplitter *main);

            void buildContentArea (QSplitter *main);

        public:

            Dialogue();

        protected:

            void closeEvent (QCloseEvent *event);

        public slots:

            void show();
    };
}

#endif
