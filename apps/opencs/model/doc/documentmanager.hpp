#ifndef CSM_DOC_DOCUMENTMGR_H
#define CSM_DOC_DOCUMENTMGR_H

#include <vector>
#include <string>

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

            Document *addDocument (const std::string& name);
            ///< The ownership of the returned document is not transferred to the caller.

            bool removeDocument (Document *document);
            ///< \return last document removed?
    };
}

#endif