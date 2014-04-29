#ifndef CSV_DOC_LOADER_H
#define CSV_DOC_LOADER_H

#include <map>

#include <QObject>
#include <QWidget>

namespace CSMDoc
{
    class Document;
}

namespace CSVDoc
{
    class LoadingDocument : public QWidget
    {
            Q_OBJECT

        public:

            LoadingDocument (CSMDoc::Document *document);
    };

    class Loader : public QObject
    {
            Q_OBJECT

            std::map<CSMDoc::Document *, LoadingDocument *> mDocuments;

        public:

            Loader();

            virtual ~Loader();

        public slots:

            void add (CSMDoc::Document *document, bool new_);

            void loadingStopped (CSMDoc::Document *document, bool completed,
                const std::string& error);
    };
}

#endif
