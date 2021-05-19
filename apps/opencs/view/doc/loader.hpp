#ifndef CSV_DOC_LOADER_H
#define CSV_DOC_LOADER_H

#include <map>

#include <QObject>
#include <QWidget>
#include <QSignalMapper>

class QLabel;
class QProgressBar;
class QDialogButtonBox;
class QListWidget;
class QVBoxLayout;

namespace CSMDoc
{
    class Document;
}

namespace CSVDoc
{
    class LoadingDocument : public QWidget
    {
            Q_OBJECT

            CSMDoc::Document *mDocument;
            QLabel *mFile;
            QLabel *mRecords;
            QProgressBar *mFileProgress;
            QProgressBar *mRecordProgress;
            bool mAborted;
            QDialogButtonBox *mButtons;
            QLabel *mError;
            QListWidget *mMessages;
            QVBoxLayout *mLayout;
            int mTotalRecords;

        private:

            void closeEvent (QCloseEvent *event) override;

        public:

            LoadingDocument (CSMDoc::Document *document);

            void nextStage (const std::string& name, int totalRecords);

            void nextRecord (int records);

            void abort (const std::string& error);

            void addMessage (const std::string& message);

        private slots:

            void cancel();

        signals:

            void cancel (CSMDoc::Document *document);
            ///< Stop loading process.

            void close (CSMDoc::Document *document);
            ///< Close stopped loading process.
    };

    class Loader : public QObject
    {
            Q_OBJECT

            std::map<CSMDoc::Document *, LoadingDocument *> mDocuments;

        public:

            Loader();

            ~Loader() override;

        signals:

            void cancel (CSMDoc::Document *document);

            void close (CSMDoc::Document *document);

        public slots:

            void add (CSMDoc::Document *document);

            void loadingStopped (CSMDoc::Document *document, bool completed,
                const std::string& error);

            void nextStage (CSMDoc::Document *document, const std::string& name, int totalRecords);

            void nextRecord (CSMDoc::Document *document, int records);

            void loadMessage (CSMDoc::Document *document, const std::string& message);
    };
}

#endif
