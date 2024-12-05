/// Main header for reading .nif files

#ifndef OPENMW_COMPONENTS_NIF_NIFFILE_HPP
#define OPENMW_COMPONENTS_NIF_NIFFILE_HPP

#include <atomic>
#include <cstdint>
#include <vector>

#include <components/files/istreamptr.hpp>
#include <components/vfs/pathutil.hpp>

#include "record.hpp"

namespace ToUTF8
{
    class StatelessUtf8Encoder;
}

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
            BETHVER_SKY = 83, // Skyrim
            BETHVER_SSE = 100, // Skyrim SE
            BETHVER_FO4 = 130, // Fallout 4
            BETHVER_F76 = 155, // Fallout 76
            BETHVER_STF = 172, // Starfield
        };

        /// File version, user version, Bethesda version
        std::uint32_t mVersion = 0;
        std::uint32_t mUserVersion = 0;
        std::uint32_t mBethVersion = 0;

        /// File name, used for error messages and opening the file
        VFS::Path::Normalized mPath;
        std::string mHash;

        /// Record list
        std::vector<std::unique_ptr<Record>> mRecords;

        /// Root list.  This is a select portion of the pointers from records
        std::vector<Record*> mRoots;

        bool mUseSkinning = false;

        explicit NIFFile(VFS::Path::NormalizedView path)
            : mPath(path)
        {
        }
    };

    class FileView
    {
    public:
        FileView(const NIFFile& file)
            : mFile(&file)
        {
        }

        /// Get a given root
        const Record* getRoot(std::size_t index) const { return mFile->mRoots.at(index); }

        /// Number of roots
        std::size_t numRoots() const { return mFile->mRoots.size(); }

        /// Get the name of the file
        const std::string& getFilename() const { return mFile->mPath; }

        const std::string& getHash() const { return mFile->mHash; }

        /// Get the version of the NIF format used
        std::uint32_t getVersion() const { return mFile->mVersion; }

        /// Get the user version of the NIF format used
        std::uint32_t getUserVersion() const { return mFile->mUserVersion; }

        /// Get the Bethesda version of the NIF format used
        std::uint32_t getBethVersion() const { return mFile->mBethVersion; }

        bool getUseSkinning() const { return mFile->mUseSkinning; }

    private:
        const NIFFile* mFile;
    };

    class Reader
    {
        /// File version, user version, Bethesda version
        std::uint32_t& mVersion;
        std::uint32_t& mUserVersion;
        std::uint32_t& mBethVersion;

        /// File name, used for error messages and opening the file
        std::string_view mFilename;
        std::string& mHash;

        /// Record list
        std::vector<std::unique_ptr<Record>>& mRecords;

        /// Root list.  This is a select portion of the pointers from records
        std::vector<Record*>& mRoots;

        /// String table
        std::vector<std::string> mStrings;

        bool& mUseSkinning;
        const ToUTF8::StatelessUtf8Encoder* mEncoder;

        static std::atomic_bool sLoadUnsupportedFiles;
        static std::atomic_bool sWriteNifDebugLog;

        /// Get the file's version in a human readable form
        ///\returns A string containing a human readable NIF version number
        std::string versionToString(std::uint32_t version);

    public:
        /// Open a NIF stream. The name is used for error messages.
        explicit Reader(NIFFile& file, const ToUTF8::StatelessUtf8Encoder* encoder);

        /// Parse the file
        void parse(Files::IStreamPtr&& stream);

        /// Get a given record
        Record* getRecord(size_t index) const { return mRecords.at(index).get(); }

        /// Get a given string from the file's string table
        std::string getString(std::uint32_t index) const;

        /// Set whether there is skinning contained in this NIF file.
        /// @note This is just a hint for users of the NIF file and has no effect on the loading procedure.
        void setUseSkinning(bool skinning);

        /// Get the name of the file
        std::string_view getFilename() const { return mFilename; }

        /// Get the version of the NIF format used
        std::uint32_t getVersion() const { return mVersion; }

        /// Get the user version of the NIF format used
        std::uint32_t getUserVersion() const { return mUserVersion; }

        /// Get the Bethesda version of the NIF format used
        std::uint32_t getBethVersion() const { return mBethVersion; }

        static void setLoadUnsupportedFiles(bool load);

        static void setWriteNifDebugLog(bool load);
    };
    using NIFFilePtr = std::shared_ptr<const Nif::NIFFile>;

} // Namespace
#endif
