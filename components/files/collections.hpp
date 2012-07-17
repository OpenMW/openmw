#ifndef COMPONENTS_FILES_COLLECTION_HPP
#define COMPONENTS_FILES_COLLECTION_HPP

#include <boost/filesystem.hpp>

#include "multidircollection.hpp"

namespace Files
{
    class Collections
    {
        public:
            Collections();

            ///< Directories are listed with increasing priority.
            Collections(const Files::PathContainer& directories, bool foldCase);

            ///< Return a file collection for the given extension. Extension must contain the
            /// leading dot and must be all lower-case.
            const MultiDirCollection& getCollection(const std::string& extension) const;

            const Files::PathContainer& getPaths() const;

        private:
            typedef std::map<std::string, MultiDirCollection> MultiDirCollectionContainer;
            Files::PathContainer mDirectories;

            bool mFoldCase;
            mutable MultiDirCollectionContainer mCollections;
    };
}

#endif
