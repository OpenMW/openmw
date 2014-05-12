#ifndef CSM_DOC_DOCUMENTMGR_H
#define CSM_DOC_DOCUMENTMGR_H

#include <vector>
#include <string>

#include <boost/filesystem/path.hpp>

#include <QObject>
#include <QThread>

#include <components/to_utf8/to_utf8.hpp>

#include "loader.hpp"

namespace Files
{
    class ConfigurationManager;
}

namespace CSMDoc
{
    class Document;

    class DocumentManager : public QObject
    {
            Q_OBJECT

            std::vector<Document *> mDocuments;
            const Files::ConfigurationManager& mConfiguration;
            QThread mLoaderThread;
            Loader mLoader;
            ToUTF8::FromType mEncoding;

            DocumentManager (const DocumentManager&);
            DocumentManager& operator= (const DocumentManager&);

        public:

            DocumentManager (const Files::ConfigurationManager& configuration);

            ~DocumentManager();

            void addDocument (const std::vector< boost::filesystem::path >& files,
                const boost::filesystem::path& savePath, bool new_);
            ///< \param new_ Do not load the last content file in \a files and instead create in an
            /// appropriate way.

	    void setResourceDir (const boost::filesystem::path& parResDir);

            void setEncoding (ToUTF8::FromType encoding);

        private:

	    boost::filesystem::path mResDir;

        private slots:

            void documentLoaded (Document *document);
            ///< The ownership of \a document is not transferred.

            void documentNotLoaded (Document *document, const std::string& error);
            ///< Document load has been interrupted either because of a call to abortLoading
            /// or a problem during loading). In the former case error will be an empty string.

        public slots:

            void removeDocument (CSMDoc::Document *document);
            ///< Emits the lastDocumentDeleted signal, if applicable.

        signals:

            void documentAdded (CSMDoc::Document *document);

            void loadRequest (CSMDoc::Document *document);

            void lastDocumentDeleted();

            void loadingStopped (CSMDoc::Document *document, bool completed,
                const std::string& error);

            void nextStage (CSMDoc::Document *document, const std::string& name, int steps);

            void nextRecord (CSMDoc::Document *document);

            void cancelLoading (CSMDoc::Document *document);

            void loadMessage (CSMDoc::Document *document, const std::string& message);
    };
}

#endif