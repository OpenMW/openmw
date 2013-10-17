#ifndef CSM_DOC_DOCUMENTMGR_H
#define CSM_DOC_DOCUMENTMGR_H

#include <vector>
#include <string>

#include <boost/filesystem/path.hpp>

namespace CSMDoc
{
    class Document;

    class DocumentManager
    {
            std::vector<Document *> mDocuments;

            DocumentManager (const DocumentManager&);
            DocumentManager& operator= (const DocumentManager&);

        public:

            DocumentManager();

            ~DocumentManager();

            Document *addDocument (const std::vector<boost::filesystem::path>& files, bool new_);
            ///< The ownership of the returned document is not transferred to the caller.
            ///
            /// \param new_ Do not load the last content file in \a files and instead create in an
            /// appropriate way.

            bool removeDocument (Document *document);
            ///< \return last document removed?
    };
}

#endif