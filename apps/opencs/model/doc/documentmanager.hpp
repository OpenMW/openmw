#ifndef CSM_DOC_DOCUMENTMGR_H
#define CSM_DOC_DOCUMENTMGR_H

#include <vector>
#include <string>

#include <boost/filesystem/path.hpp>

namespace Files
{
    class ConfigurationManager;
}

namespace CSMDoc
{
    class Document;

    class DocumentManager
    {
            std::vector<Document *> mDocuments;
            const Files::ConfigurationManager& mConfiguration;

            DocumentManager (const DocumentManager&);
            DocumentManager& operator= (const DocumentManager&);

        public:

            DocumentManager (const Files::ConfigurationManager& configuration);

            ~DocumentManager();

            Document *addDocument (const std::vector< boost::filesystem::path >& files,
                                   const boost::filesystem::path& savePath,
                                   bool new_);
            ///< The ownership of the returned document is not transferred to the caller.
            ///
            /// \param new_ Do not load the last content file in \a files and instead create in an
            /// appropriate way.

            bool removeDocument (Document *document);
            ///< \return last document removed?
	    void setResourceDir (const boost::filesystem::path& parResDir);
	    
    private:
	    boost::filesystem::path mResDir;
    };
}

#endif