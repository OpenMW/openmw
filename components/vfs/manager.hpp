#ifndef OPENMW_COMPONENTS_RESOURCEMANAGER_H
#define OPENMW_COMPONENTS_RESOURCEMANAGER_H

#include <components/files/constrainedfilestream.hpp>

#include <vector>
#include <map>

namespace VFS
{

    class Archive;
    class File;

    class RecursiveDirectoryIterator;
    RecursiveDirectoryIterator end(const RecursiveDirectoryIterator& iter);

    class RecursiveDirectoryIterator
    {
    public:
        RecursiveDirectoryIterator(const std::map<std::string, File*>& index, const std::string& path)
            : mPath(path)
            , mIndex(&index)
            , mIt(index.lower_bound(path))
        {}

        RecursiveDirectoryIterator(const RecursiveDirectoryIterator&) = default;

        const std::string& operator*() const { return mIt->first; }
        const std::string* operator->() const { return &mIt->first; }

        bool operator!=(const RecursiveDirectoryIterator& other) { return mPath != other.mPath || mIt != other.mIt; }

        RecursiveDirectoryIterator& operator++()
        {
            if (++mIt == mIndex->end() || !starts_with(mIt->first, mPath))
                *this = end(*this);
            return *this;
        }

        friend RecursiveDirectoryIterator end(const RecursiveDirectoryIterator& iter);

    private:
        static bool starts_with(const std::string& text, const std::string& start) { return text.rfind(start, 0) == 0; }

        std::string mPath;
        const std::map<std::string, File*>* mIndex;
        std::map<std::string, File*>::const_iterator mIt;
    };

    inline RecursiveDirectoryIterator begin(RecursiveDirectoryIterator iter) { return iter; }

    inline RecursiveDirectoryIterator end(const RecursiveDirectoryIterator& iter)
    {
        RecursiveDirectoryIterator result(iter);
        result.mIt = result.mIndex->end();
        return result;
    }

    /// @brief The main class responsible for loading files from a virtual file system.
    /// @par Various archive types (e.g. directories on the filesystem, or compressed archives)
    /// can be registered, and will be merged into a single file tree. If the same filename is
    /// contained in multiple archives, the last added archive will have priority.
    /// @par Most of the methods in this class are considered thread-safe, see each method documentation for details.
    class Manager
    {
    public:
        /// @param strict Use strict path handling? If enabled, no case folding will
        /// be done, but slash/backslash conversions are always done.
        Manager(bool strict);

        ~Manager();

        // Empty the file index and unregister archives.
        void reset();

        /// Register the given archive. All files contained in it will be added to the index on the next buildIndex() call.
        /// @note Takes ownership of the given pointer.
        void addArchive(Archive* archive);

        /// Build the file index. Should be called when all archives have been registered.
        void buildIndex();

        /// Does a file with this name exist?
        /// @note May be called from any thread once the index has been built.
        bool exists(const std::string& name) const;

        /// Normalize the given filename, making slashes/backslashes consistent, and lower-casing if mStrict is false.
        /// @note May be called from any thread once the index has been built.
        [[nodiscard]] std::string normalizeFilename(const std::string& name) const;

        /// Retrieve a file by name.
        /// @note Throws an exception if the file can not be found.
        /// @note May be called from any thread once the index has been built.
        Files::IStreamPtr get(const std::string& name) const;

        /// Retrieve a file by name (name is already normalized).
        /// @note Throws an exception if the file can not be found.
        /// @note May be called from any thread once the index has been built.
        Files::IStreamPtr getNormalized(const std::string& normalizedName) const;

        std::string getArchive(const std::string& name) const;

        /// Recursivly iterate over the elements of the given path
        /// In practice it return all files of the VFS starting with the given path
        /// @note the path is normalized
        /// @note May be called from any thread once the index has been built.
        RecursiveDirectoryIterator getRecursiveDirectoryIterator(const std::string& path) const;

    private:
        bool mStrict;

        std::vector<Archive*> mArchives;

        std::map<std::string, File*> mIndex;
    };

}

#endif
