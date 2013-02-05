#ifndef CSV_DOC_STARTUP_H
#define CSV_DOC_STARTUP_H

#include <QWidget>

namespace CSVDoc
{
    class StartupDialogue : public QWidget
    {
        Q_OBJECT

        public:

            StartupDialogue();

        signals:

            void createDocument();

            void loadDocument();
    };
}

#endif
