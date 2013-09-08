#ifndef CSV_DOC_NEWGAME_H
#define CSV_DOC_NEWGAME_H

#include <QDialog>

class QPushButton;

namespace CSVDoc
{
    class FileWidget;

    class NewGameDialogue : public QDialog
    {
            Q_OBJECT

            QPushButton *mCreate;
            FileWidget *mFileWidget;

        public:

            NewGameDialogue();

        signals:

            void createRequest (const QString& file);

        private slots:

            void stateChanged (bool valid);

            void create();
    };
}

#endif
