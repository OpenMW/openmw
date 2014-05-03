#ifndef CSV_DOC_LOADER_H
#define CSV_DOC_LOADER_H

#include <map>

#include <QObject>
#include <QWidget>

class QLabel;
class QProgressBar;

namespace CSMDoc
{
    class Document;
}

namespace CSVDoc
{
    class LoadingDocument : public QWidget
    {
            Q_OBJECT

            QLabel *mFile;
            QProgressBar *mFileProgress;

        public:

            LoadingDocument (CSMDoc::Document *document);

            void nextStage (const std::string& name);
    };

    class Loader : public QObject
    {
            Q_OBJECT

            std::map<CSMDoc::Document *, LoadingDocument *> mDocuments;

        public:

            Loader();

            virtual ~Loader();

        public slots:

            void add (CSMDoc::Document *document);

            void loadingStopped (CSMDoc::Document *document, bool completed,
                const std::string& error);

            void nextStage (CSMDoc::Document *document, const std::string& name);
    };
}

#endif
