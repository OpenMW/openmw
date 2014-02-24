#ifndef CSV_DOC_STARTUP_H
#define CSV_DOC_STARTUP_H

#include <QWidget>

class QGridLayout;
class QString;
class QPushButton;
class QWidget;
class QIcon;

namespace CSVDoc
{
    class StartupDialogue : public QWidget
    {
        Q_OBJECT

        private:

            int mWidth;
            int mColumn;
            QGridLayout *mLayout;

            QPushButton *addButton (const QString& label, const QIcon& icon);

            QWidget *createButtons();

            QWidget *createTools();

        public:

            StartupDialogue();

        signals:

            void createGame();

            void createAddon();

            void loadDocument();

            void editConfig();
    };
}

#endif
