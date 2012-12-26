#ifndef COMPONENTS_FILES_MULTIDIRSOLLECTION_HPP
#define COMPONENTS_FILES_MULTIDIRSOLLECTION_HPP

#include <map>
#include <vector>
#include <string>
#include <locale>
#include <cctype>

#include <boost/filesystem/path.hpp>

namespace Files
{
    typedef std::vector<boost::filesystem::path> PathContainer;

    struct NameLess
    {
        bool mStrict;

        NameLess (bool strict) : mStrict (strict) {}

        bool operator() (const std::string& left, const std::string& right) const
        {
            if (mStrict)
                return left<right;

            std::size_t min = std::min (left.length(), right.length());
            std::locale loc;

            for (std::size_t i=0; i<min; ++i)
            {
                char l = std::tolower (left[i], loc);
                char r = std::tolower (right[i], loc);

                if (l<r)
                    return true;
                if (l>r)
                    return false;
            }

            return left.length()<right.length();
        }
    };

    /// \brief File collection across several directories
    ///
    /// This class lists all files with one specific extensions within one or more
    /// directories. If the same file appears more than once, the file in the directory
    /// with the higher priority is used.
    class MultiDirCollection
    {
        public:

            typedef std::map<std::string, boost::filesystem::path, NameLess> TContainer;
            typedef TContainer::const_iterator TIter;

        private:

            TContainer mFiles;

        public:

            MultiDirCollection (const Files::PathContainer& directories,
                const std::string& extension, bool foldCase);
            ///< Directories are listed with increasing priority.
            /// \param extension The extension that should be listed in this collection. Must
            /// contain the leading dot.
            /// \param foldCase Ignore filename case

            boost::filesystem::path getPath (const std::string& file) const;
            ///< Return full path (including filename) of \a file.
            ///
            /// If the file does not exist, an exception is thrown. \a file must include
            /// the extension.

            bool doesExist (const std::string& file) const;
            ///< \return Does a file with the given name exist?

            TIter begin() const;
            ///< Return iterator pointing to the first file.

            TIter end() const;
            ///< Return iterator pointing past the last file.

    };
}

#endif
