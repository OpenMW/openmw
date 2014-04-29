#ifndef CSM_DOC_LOADER_H
#define CSM_DOC_LOADER_H

#include <vector>

#include <QObject>
#include <QMutex>
#include <QWaitCondition>

namespace CSMDoc
{
    class Document;

    class Loader : public QObject
    {
            Q_OBJECT

            QMutex mMutex;
            QWaitCondition mThingsToDo;
            std::vector<std::pair<Document *, bool> > mDocuments;

        public:

            Loader();

            QWaitCondition& hasThingsToDo();

        private slots:

            void load();

        public slots:

            void loadDocument (CSMDoc::Document *document, bool new_);
            ///< The ownership of \a document is not transferred.
            /// \param new_ Do not load the last content file in the files list specified in
            /// \a document and instead create it in an appropriate way.

            void abortLoading (Document *document);
            ///< Abort loading \a docuemnt (ignored if \a document has already finished being
            /// loaded). Will result in a documentNotLoaded signal, once the Loader has finished
            /// cleaning up.

        signals:

            void documentLoaded (Document *document);
            ///< The ownership of \a document is not transferred.

            void documentNotLoaded (Document *document, const std::string& error);
            ///< Document load has been interrupted either because of a call to abortLoading
            /// or a problem during loading). In the former case error will be an empty string.
    };
}

#endif
