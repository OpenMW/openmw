#ifndef CSM_DOC_LOADER_H
#define CSM_DOC_LOADER_H

#include <vector>

#include <QObject>
#include <QMutex>
#include <QTimer>
#include <QWaitCondition>

namespace CSMDoc
{
    class Document;

    class Loader : public QObject
    {
            Q_OBJECT

            struct Stage
            {
                int mFile;
                int mRecordsLoaded;
                bool mRecordsLeft;

                Stage();
            };

            QMutex mMutex;
            QWaitCondition mThingsToDo;
            std::vector<std::pair<Document *, Stage> > mDocuments;

            QTimer* mTimer;
            bool mShouldStop;

        public:

            Loader();

            QWaitCondition& hasThingsToDo();

            void stop();

        private slots:

            void load();

        public slots:

            void loadDocument (CSMDoc::Document *document);
            ///< The ownership of \a document is not transferred.

            void abortLoading (CSMDoc::Document *document);
            ///< Abort loading \a docuemnt (ignored if \a document has already finished being
            /// loaded). Will result in a documentNotLoaded signal, once the Loader has finished
            /// cleaning up.

        signals:

            void documentLoaded (Document *document);
            ///< The ownership of \a document is not transferred.

            void documentNotLoaded (Document *document, const std::string& error);
            ///< Document load has been interrupted either because of a call to abortLoading
            /// or a problem during loading). In the former case error will be an empty string.

            void nextStage (CSMDoc::Document *document, const std::string& name,
                int totalRecords);

            void nextRecord (CSMDoc::Document *document, int records);
            ///< \note This signal is only given once per group of records. The group size is
            /// approximately the total number of records divided by the steps value of the
            /// previous nextStage signal.

            void loadMessage (CSMDoc::Document *document, const std::string& message);
            ///< Non-critical load error or warning
    };
}

#endif
