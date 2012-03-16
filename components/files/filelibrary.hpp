#ifndef COMPONENTS_FILES_FILELIBRARY_HPP
#define COMPONENTS_FILES_FILELIBRARY_HPP

#include <components/files/fileops.hpp>

namespace Files
{
    typedef std::map<std::string, PathContainer> StringPathContMap;
    typedef std::vector<std::string> StringVector;

    /// Looks for a string in a vector of strings
    bool containsVectorString(const StringVector& list, const std::string& str);

    /// \brief Searches directories and makes lists of files according to folder name
    class FileLibrary
    {
        private:
            StringPathContMap mMap;
            PathContainer mEmptyPath;
            PathContainer mPriorityList;

        public:
            /// Searches a path and adds the results to the library
            /// Recursive search and fs strict options are available
            /// Takes a vector of acceptable files extensions, if none is given it lists everything.
            void add(const boost::filesystem::path &root, bool recursive, bool strict,
                     const StringVector &acceptableExtensions);

            /// Returns true if the named section exists
            /// You can run this check before running section()
            bool containsSection(std::string sectionName, bool strict);

            /// Returns a pointer to const for a section of the library
            /// which is essentially a PathContainer.
            /// If the section does not exists it returns a pointer to an empty path.
            const PathContainer* section(std::string sectionName, bool strict);

            /// Searches the library for an item and returns a boost path to it
            /// Optionally you can provide a specific section
            /// The result is the first that comes up according to alphabetical
            /// section naming
            boost::filesystem::path locate(std::string item, bool strict, bool ignoreExtensions, std::string sectionName="");

            /// Prints all the available sections, used for debugging
            void printSections();
    };
}

#endif
