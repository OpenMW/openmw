#ifndef CSM_DOC_DOCUMENTMGR_H
#define CSM_DOC_DOCUMENTMGR_H

#include <vector>
#include <string>

#include <boost/filesystem/path.hpp>

#include <QObject>
#include <QThread>

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

            DocumentManager (const DocumentManager&);
            DocumentManager& operator= (const DocumentManager&);

        public:

            DocumentManager (const Files::ConfigurationManager& configuration);

            ~DocumentManager();

            void addDocument (const std::vector< boost::filesystem::path >& files,
                const boost::filesystem::path& savePath, bool new_);
            ///< \param new_ Do not load the last content file in \a files and instead create in an
            /// appropriate way.

            void removeDocument (Document *document);
            ///< Emits the lastDocumentDeleted signal, if applicable.

	    void setResourceDir (const boost::filesystem::path& parResDir);

        private:

	    boost::filesystem::path mResDir;

        private slots:

            void documentLoaded (Document *document);
            ///< The ownership of \a document is not transferred.

            void documentNotLoaded (Document *document, const std::string& error);
            ///< Document load has been interrupted either because of a call to abortLoading
            /// or a problem during loading). In the former case error will be an empty string.

        signals:

            void documentAdded (CSMDoc::Document *document);

            void loadRequest (CSMDoc::Document *document, bool _new);

            void lastDocumentDeleted();

            void loadingStopped (CSMDoc::Document *document, bool completed,
                const std::string& error);
    };
}

#endif