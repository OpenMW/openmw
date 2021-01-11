#ifndef OPENMW_COMPONENTS_RESOURCEMANAGER_H
#define OPENMW_COMPONENTS_RESOURCEMANAGER_H

#include <components/files/constrainedfilestream.hpp>

#include <vector>
#include <map>

namespace VFS
{

    class Archive;
    class File;

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

        /// Get a complete list of files from all archives
        /// @note May be called from any thread once the index has been built.
        const std::map<std::string, File*>& getIndex() const;

        /// Normalize the given filename, making slashes/backslashes consistent, and lower-casing if mStrict is false.
        /// @note May be called from any thread once the index has been built.
        void normalizeFilename(std::string& name) const;

        /// Retrieve a file by name.
        /// @note Throws an exception if the file can not be found.
        /// @note May be called from any thread once the index has been built.
        Files::IStreamPtr get(const std::string& name) const;

        /// Retrieve a file by name (name is already normalized).
        /// @note Throws an exception if the file can not be found.
        /// @note May be called from any thread once the index has been built.
        Files::IStreamPtr getNormalized(const std::string& normalizedName) const;

        std::string getArchive(const std::string& name) const;
    private:
        bool mStrict;

        std::vector<Archive*> mArchives;

        std::map<std::string, File*> mIndex;
    };

}

#endif
