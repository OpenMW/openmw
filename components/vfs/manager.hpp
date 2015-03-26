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
    class Manager
    {
    public:
        /// @param strict Use strict path handling? If enabled, no case folding will
        /// be done, but slash/backslash conversions are always done.
        Manager(bool strict);

        ~Manager();

        /// Register the given archive. All files contained in it will be added to the index on the next buildIndex() call.
        /// @note Takes ownership of the given pointer.
        void addArchive(Archive* archive);

        /// Build the file index. Should be called when all archives have been registered.
        void buildIndex();

        /// Does a file with this name exist?
        bool exists(const std::string& name) const;

        /// Get a complete list of files from all archives
        const std::map<std::string, File*>& getIndex() const;

        /// Normalize the given filename, making slashes/backslashes consistent, and lower-casing if mStrict is false.
        void normalizeFilename(std::string& name) const;

        /// Retrieve a file by name.
        /// @note Throws an exception if the file can not be found.
        Files::IStreamPtr get(const std::string& name) const;

        /// Retrieve a file by name (name is already normalized).
        /// @note Throws an exception if the file can not be found.
        Files::IStreamPtr getNormalized(const std::string& normalizedName) const;

    private:
        bool mStrict;

        std::vector<Archive*> mArchives;

        std::map<std::string, File*> mIndex;
    };

}

#endif
