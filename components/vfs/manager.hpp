#ifndef OPENMW_COMPONENTS_RESOURCEMANAGER_H
#define OPENMW_COMPONENTS_RESOURCEMANAGER_H

#include <components/files/istreamptr.hpp>

#include <filesystem>
#include <map>
#include <memory>
#include <string>
#include <vector>

namespace VFS
{

    class Archive;
    class File;

    template <typename Iterator>
    class IteratorPair
    {
    public:
        IteratorPair(Iterator first, Iterator last)
            : mFirst(first)
            , mLast(last)
        {
        }
        Iterator begin() const { return mFirst; }
        Iterator end() const { return mLast; }

    private:
        Iterator mFirst;
        Iterator mLast;
    };

    /// @brief The main class responsible for loading files from a virtual file system.
    /// @par Various archive types (e.g. directories on the filesystem, or compressed archives)
    /// can be registered, and will be merged into a single file tree. If the same filename is
    /// contained in multiple archives, the last added archive will have priority.
    /// @par Most of the methods in this class are considered thread-safe, see each method documentation for details.
    class Manager
    {
        class RecursiveDirectoryIterator
        {
        public:
            RecursiveDirectoryIterator(std::map<std::string, File*>::const_iterator it)
                : mIt(it)
            {
            }
            const std::string& operator*() const { return mIt->first; }
            const std::string* operator->() const { return &mIt->first; }
            bool operator!=(const RecursiveDirectoryIterator& other) { return mIt != other.mIt; }
            RecursiveDirectoryIterator& operator++()
            {
                ++mIt;
                return *this;
            }

        private:
            std::map<std::string, File*>::const_iterator mIt;
        };

        using RecursiveDirectoryRange = IteratorPair<RecursiveDirectoryIterator>;

    public:
        /// @param strict Use strict path handling? If enabled, no case folding will
        /// be done, but slash/backslash conversions are always done.
        Manager(bool strict);

        ~Manager();

        // Empty the file index and unregister archives.
        void reset();

        /// Register the given archive. All files contained in it will be added to the index on the next buildIndex()
        /// call.
        void addArchive(std::unique_ptr<Archive>&& archive);

        /// Build the file index. Should be called when all archives have been registered.
        void buildIndex();

        /// Does a file with this name exist?
        /// @note May be called from any thread once the index has been built.
        bool exists(std::string_view name) const;

        /// Normalize the given filename, making slashes/backslashes consistent, and lower-casing if mStrict is false.
        /// @note May be called from any thread once the index has been built.
        [[nodiscard]] std::string normalizeFilename(std::string_view name) const;

        /// Retrieve a file by name.
        /// @note Throws an exception if the file can not be found.
        /// @note May be called from any thread once the index has been built.
        Files::IStreamPtr get(std::string_view name) const;

        /// Retrieve a file by name (name is already normalized).
        /// @note Throws an exception if the file can not be found.
        /// @note May be called from any thread once the index has been built.
        Files::IStreamPtr getNormalized(const std::string& normalizedName) const;

        std::string getArchive(std::string_view name) const;

        /// Recursively iterate over the elements of the given path
        /// In practice it return all files of the VFS starting with the given path
        /// @note the path is normalized
        /// @note May be called from any thread once the index has been built.
        RecursiveDirectoryRange getRecursiveDirectoryIterator(std::string_view path) const;

        /// Retrieve the absolute path to the file
        /// @note Throws an exception if the file can not be found.
        /// @note May be called from any thread once the index has been built.
        std::filesystem::path getAbsoluteFileName(const std::filesystem::path& name) const;

    private:
        bool mStrict;

        std::vector<std::unique_ptr<Archive>> mArchives;

        std::map<std::string, File*> mIndex;
    };

}

#endif
