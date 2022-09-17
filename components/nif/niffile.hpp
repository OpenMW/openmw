/// Main header for reading .nif files

#ifndef OPENMW_COMPONENTS_NIF_NIFFILE_HPP
#define OPENMW_COMPONENTS_NIF_NIFFILE_HPP

#include <atomic>
#include <filesystem>
#include <vector>

#include <components/files/istreamptr.hpp>

#include "record.hpp"

namespace Nif
{

    struct NIFFile
    {
        // For generic versions NIFStream::generateVersion() is used instead
        enum NIFVersion
        {
            VER_MW = 0x04000002, // 4.0.0.2. Main Morrowind NIF version.
            VER_OB_OLD = 0x0A000102, // 10.0.1.2. Main older Oblivion NIF version.
            VER_OB = 0x14000005, // 20.0.0.5. Main Oblivion NIF version.
            VER_BGS = 0x14020007 // 20.2.0.7. Main Fallout 3/4/76/New Vegas and Skyrim/SkyrimSE NIF version.
        };
        enum BethVersion
        {
            BETHVER_FO3 = 34, // Fallout 3
            BETHVER_FO4 = 130 // Fallout 4
        };

        /// File version, user version, Bethesda version
        unsigned int mVersion = 0;
        unsigned int mUserVersion = 0;
        unsigned int mBethVersion = 0;

        /// File name, used for error messages and opening the file
        std::filesystem::path mPath;
        std::string mHash;

        /// Record list
        std::vector<std::unique_ptr<Record>> mRecords;

        /// Root list.  This is a select portion of the pointers from records
        std::vector<Record*> mRoots;

        bool mUseSkinning = false;

        explicit NIFFile(const std::filesystem::path& path)
            : mPath(path)
        {
        }

        /// Get a given root
        Record* getRoot(size_t index = 0) const { return mRoots.at(index); }

        /// Number of roots
        std::size_t numRoots() const { return mRoots.size(); }

        /// Get the name of the file
        const std::filesystem::path& getFilename() const { return mPath; }

        const std::string& getHash() const { return mHash; }

        /// Get the version of the NIF format used
        unsigned int getVersion() const { return mVersion; }

        /// Get the user version of the NIF format used
        unsigned int getUserVersion() const { return mUserVersion; }

        /// Get the Bethesda version of the NIF format used
        unsigned int getBethVersion() const { return mBethVersion; }

        bool getUseSkinning() const { return mUseSkinning; }
    };

    class Reader
    {
        /// File version, user version, Bethesda version
        unsigned int& ver;
        unsigned int& userVer;
        unsigned int& bethVer;

        /// File name, used for error messages and opening the file
        std::filesystem::path& filename;
        std::string& hash;

        /// Record list
        std::vector<std::unique_ptr<Record>>& records;

        /// Root list.  This is a select portion of the pointers from records
        std::vector<Record*>& roots;

        /// String table
        std::vector<std::string> strings;

        bool& mUseSkinning;

        static std::atomic_bool sLoadUnsupportedFiles;

        /// Get the file's version in a human readable form
        ///\returns A string containing a human readable NIF version number
        std::string printVersion(unsigned int version);

    public:
        /// Open a NIF stream. The name is used for error messages.
        explicit Reader(NIFFile& file);

        /// Parse the file
        void parse(Files::IStreamPtr&& stream);

        /// Get a given record
        Record* getRecord(size_t index) const { return records.at(index).get(); }

        /// Get a given string from the file's string table
        std::string getString(uint32_t index) const;

        /// Set whether there is skinning contained in this NIF file.
        /// @note This is just a hint for users of the NIF file and has no effect on the loading procedure.
        void setUseSkinning(bool skinning);

        /// Get the name of the file
        std::filesystem::path getFilename() const { return filename; }

        /// Get the version of the NIF format used
        unsigned int getVersion() const { return ver; }

        /// Get the user version of the NIF format used
        unsigned int getUserVersion() const { return userVer; }

        /// Get the Bethesda version of the NIF format used
        unsigned int getBethVersion() const { return bethVer; }

        static void setLoadUnsupportedFiles(bool load);
    };
    using NIFFilePtr = std::shared_ptr<const Nif::NIFFile>;

} // Namespace
#endif
