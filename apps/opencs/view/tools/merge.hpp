#ifndef CSV_TOOLS_REPORTTABLE_H
#define CSV_TOOLS_REPORTTABLE_H

#include <QDialog>

#include <boost/filesystem/path.hpp>

class QPushButton;
class QListWidget;

namespace CSMDoc
{
    class Document;
}

namespace CSVDoc
{
    class FileWidget;
    class AdjusterWidget;
}

namespace CSVTools
{
    class Merge : public QDialog
    {
            Q_OBJECT

            CSMDoc::Document *mDocument;
            QPushButton *mOkay;
            QListWidget *mFiles;
            CSVDoc::FileWidget *mNewFile;
            CSVDoc::AdjusterWidget *mAdjuster;

        public:

            Merge (QWidget *parent = 0);

            /// Configure dialogue for a new merge
            void configure (CSMDoc::Document *document);

            void setLocalData (const boost::filesystem::path& localData);

            CSMDoc::Document *getDocument() const;

            void cancel();

        public slots:

            virtual void accept();

            virtual void reject();

        private slots:

            void stateChanged (bool valid);

    };
}

#endif
