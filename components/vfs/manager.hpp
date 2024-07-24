#ifndef OPENMW_COMPONENTS_RESOURCEMANAGER_H
#define OPENMW_COMPONENTS_RESOURCEMANAGER_H

#include <components/files/istreamptr.hpp>

#include <filesystem>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

#include "filemap.hpp"
#include "pathutil.hpp"

namespace VFS
{
    class Archive;
    class RecursiveDirectoryRange;

    /// @brief The main class responsible for loading files from a virtual file system.
    /// @par Various archive types (e.g. directories on the filesystem, or compressed archives)
    /// can be registered, and will be merged into a single file tree. If the same filename is
    /// contained in multiple archives, the last added archive will have priority.
    /// @par Most of the methods in this class are considered thread-safe, see each method documentation for details.
    class Manager
    {
    public:
        Manager();

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
        bool exists(const Path::Normalized& name) const;

        bool exists(Path::NormalizedView name) const;

        // Returns open file if exists or nullptr.
        Files::IStreamPtr find(Path::NormalizedView name) const;

        /// Retrieve a file by name.
        /// @note Throws an exception if the file can not be found.
        /// @note May be called from any thread once the index has been built.
        Files::IStreamPtr get(const Path::Normalized& name) const;

        Files::IStreamPtr get(Path::NormalizedView name) const;

        /// Retrieve a file by name (name is already normalized).
        /// @note Throws an exception if the file can not be found.
        /// @note May be called from any thread once the index has been built.
        Files::IStreamPtr getNormalized(std::string_view normalizedName) const;

        std::string getArchive(const Path::Normalized& name) const;

        /// Recursively iterate over the elements of the given path
        /// In practice it return all files of the VFS starting with the given path
        /// @note the path is normalized
        /// @note May be called from any thread once the index has been built.
        RecursiveDirectoryRange getRecursiveDirectoryIterator(std::string_view path) const;

        RecursiveDirectoryRange getRecursiveDirectoryIterator(VFS::Path::NormalizedView path) const;

        RecursiveDirectoryRange getRecursiveDirectoryIterator() const;

        /// Retrieve the absolute path to the file
        /// @note Throws an exception if the file can not be found.
        /// @note May be called from any thread once the index has been built.
        std::filesystem::path getAbsoluteFileName(const std::filesystem::path& name) const;

    private:
        std::vector<std::unique_ptr<Archive>> mArchives;

        FileMap mIndex;

        inline Files::IStreamPtr findNormalized(std::string_view normalizedPath) const;
    };

}

#endif
