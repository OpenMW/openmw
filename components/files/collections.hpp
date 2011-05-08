#ifndef COMPONENTS_FILES_COLLECTION_HPP
#define COMPONENTS_FILES_COLLECTION_HPP

#include "multidircollection.hpp"

namespace Files
{
    class Collections
    {
            std::vector<boost::filesystem::path> mDirectories;
            bool mFoldCase;
            mutable std::map<std::string, MultiDirCollection> mCollections;

        public:

            Collections();

            Collections (const std::vector<boost::filesystem::path>& directories, bool foldCase);
            ///< Directories are listed with increasing priority.

            const MultiDirCollection& getCollection (const std::string& extension) const;
            ///< Return a file collection for the given extension. Extension must contain the
            /// leading dot and must be all lower-case.

    };
}

#endif
