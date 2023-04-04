#ifndef OPENMW_ESM_READER_H
#define OPENMW_ESM_READER_H

#include <cstdint>
#include <filesystem>
#include <istream>
#include <memory>
#include <vector>

#include <components/to_utf8/to_utf8.hpp>

#include "components/esm/esmcommon.hpp"
#include "components/esm/refid.hpp"
#include "loadtes3.hpp"

namespace ESM
{

    class ReadersCache;

    class ESMReader
    {
    public:
        ESMReader();

        /*************************************************************************
         *
         *  Information retrieval
         *
         *************************************************************************/

        int getVer() const { return mHeader.mData.version; }
        int getRecordCount() const { return mHeader.mData.records; }
        float getFVer() const { return (mHeader.mData.version == VER_12) ? 1.2f : 1.3f; }
        const std::string& getAuthor() const { return mHeader.mData.author; }
        const std::string& getDesc() const { return mHeader.mData.desc; }
        const std::vector<Header::MasterData>& getGameFiles() const { return mHeader.mMaster; }
        const Header& getHeader() const { return mHeader; }
        FormatVersion getFormatVersion() const { return mHeader.mFormatVersion; }
        const NAME& retSubName() const { return mCtx.subName; }
        uint32_t getSubSize() const { return mCtx.leftSub; }
        const std::filesystem::path& getName() const { return mCtx.filename; }
        bool isOpen() const { return mEsm != nullptr; }

        /*************************************************************************
         *
         *  Opening and closing
         *
         *************************************************************************/

        /** Save the current file position and information in a ESM_Context
            struct
         */
        ESM_Context getContext();

        /** Restore a previously saved context */
        void restoreContext(const ESM_Context& rc);

        /** Close the file, resets all information. After calling close()
            the structure may be reused to load a new file.
        */
        void close();

        /// Raw opening. Opens the file and sets everything up but doesn't
        /// parse the header.
        void openRaw(std::unique_ptr<std::istream>&& stream, const std::filesystem::path& name);

        /// Load ES file from a new stream, parses the header. Closes the
        /// currently open file first, if any.
        void open(std::unique_ptr<std::istream>&& stream, const std::filesystem::path& name);

        void open(const std::filesystem::path& file);

        void openRaw(const std::filesystem::path& filename);

        /// Get the current position in the file. Make sure that the file has been opened!
        size_t getFileOffset() const { return mEsm->tellg(); }

        // This is a quick hack for multiple esm/esp files. Each plugin introduces its own
        //  terrain palette, but ESMReader does not pass a reference to the correct plugin
        //  to the individual load() methods. This hack allows to pass this reference
        //  indirectly to the load() method.
        void setIndex(const int index) { mCtx.index = index; }
        int getIndex() const { return mCtx.index; }

        // Assign parent esX files by tracking their indices in the global list of
        // all files/readers used by the engine. This is required for correct adjustRefNum() results
        // as required for handling moved, deleted and edited CellRefs.
        /// @note Does not validate.
        void resolveParentFileIndices(ReadersCache& readers);
        const std::vector<int>& getParentFileIndices() const { return mCtx.parentFileIndices; }

        /*************************************************************************
         *
         *  Medium-level reading shortcuts
         *
         *************************************************************************/

        // Because we want to get rid of CellId, we isolate it's uses.
        ESM::RefId getCellId();

        // Read data of a given type, stored in a subrecord of a given name
        template <typename X>
        void getHNT(X& x, NAME name)
        {
            getSubNameIs(name);
            getHT(x);
        }

        // Optional version of getHNT
        template <typename X>
        void getHNOT(X& x, NAME name)
        {
            if (isNextSub(name))
                getHT(x);
        }

        // Version with extra size checking, to make sure the compiler
        // doesn't mess up our struct padding.
        template <std::size_t size, typename X>
        void getHNTSized(X& x, NAME name)
        {
            static_assert(sizeof(X) == size);
            getHNT(x, name);
        }

        template <std::size_t size, typename X>
        void getHNOTSized(X& x, NAME name)
        {
            static_assert(sizeof(X) == size);
            getHNOT(x, name);
        }

        // Get data of a given type/size, including subrecord header
        template <typename X>
        void getHT(X& x)
        {
            getSubHeader();
            if (mCtx.leftSub != sizeof(X))
                reportSubSizeMismatch(sizeof(X), mCtx.leftSub);
            getT(x);
        }

        template <typename T>
        void skipHT()
        {
            getSubHeader();
            if (mCtx.leftSub != sizeof(T))
                reportSubSizeMismatch(sizeof(T), mCtx.leftSub);
            skipT<T>();
        }

        // Version with extra size checking, to make sure the compiler
        // doesn't mess up our struct padding.
        template <std::size_t size, typename X>
        void getHTSized(X& x)
        {
            static_assert(sizeof(X) == size);
            getHT(x);
        }

        template <std::size_t size, typename T>
        void skipHTSized()
        {
            static_assert(sizeof(T) == size);
            skipHT<T>();
        }

        // Read a string by the given name if it is the next record.
        std::string getHNOString(NAME name);

        ESM::RefId getHNORefId(NAME name);

        void skipHNORefId(NAME name);

        // Read a string with the given sub-record name
        std::string getHNString(NAME name);

        RefId getHNRefId(NAME name);

        // Read a string, including the sub-record header (but not the name)
        std::string getHString();

        std::string_view getHStringView();

        RefId getRefId();

        void skipHString();

        void skipHRefId();

        // Read the given number of bytes from a subrecord
        void getHExact(void* p, int size);

        // Read the given number of bytes from a named subrecord
        void getHNExact(void* p, int size, NAME name);

        /*************************************************************************
         *
         *  Low level sub-record methods
         *
         *************************************************************************/

        // Get the next subrecord name and check if it matches the parameter
        void getSubNameIs(NAME name);

        /** Checks if the next sub record name matches the parameter. If it
            does, it is read into 'subName' just as if getSubName() was
            called. If not, the read name will still be available for future
            calls to getSubName(), isNextSub() and getSubNameIs().
         */
        bool isNextSub(NAME name);

        bool peekNextSub(NAME name);

        // Store the current subrecord name for the next call of getSubName()
        void cacheSubName() { mCtx.subCached = true; }

        // Read subrecord name. This gets called a LOT, so I've optimized it
        // slightly.
        void getSubName();

        // Skip current sub record, including header (but not including
        // name.)
        void skipHSub();

        // Skip sub record and check its size
        void skipHSubSize(int size);

        // Skip all subrecords until the given subrecord or no more subrecords remaining
        void skipHSubUntil(NAME name);

        /* Sub-record header. This updates leftRec beyond the current
           sub-record as well. leftSub contains size of current sub-record.
        */
        void getSubHeader();

        /*************************************************************************
         *
         *  Low level record methods
         *
         *************************************************************************/

        // Get the next record name
        NAME getRecName();

        // Skip the rest of this record. Assumes the name and header have
        // already been read
        void skipRecord();

        /* Read record header. This updatesleftFile BEYOND the data that
           follows the header, ie beyond the entire record. You should use
           leftRec to orient yourself inside the record itself.
        */
        void getRecHeader() { getRecHeader(mRecordFlags); }
        void getRecHeader(uint32_t& flags);

        bool hasMoreRecs() const { return mCtx.leftFile > 0; }
        bool hasMoreSubs() const { return mCtx.leftRec > 0; }

        /*************************************************************************
         *
         *  Lowest level data reading and misc methods
         *
         *************************************************************************/

        template <typename X>
        void getT(X& x)
        {
            getExact(&x, sizeof(X));
        }

        template <typename T>
        void skipT()
        {
            skip(sizeof(T));
        }

        void getExact(void* x, std::size_t size)
        {
            mEsm->read(static_cast<char*>(x), static_cast<std::streamsize>(size));
        }

        void getName(NAME& name) { getT(name); }
        void getUint(uint32_t& u) { getT(u); }

        std::string getMaybeFixedStringSize(std::size_t size);

        RefId getMaybeFixedRefIdSize(std::size_t size);

        // Read the next 'size' bytes and return them as a string. Converts
        // them from native encoding to UTF8 in the process.
        std::string_view getStringView(std::size_t size);

        RefId getRefId(std::size_t size);

        void skip(std::size_t bytes)
        {
            char buffer[4096];
            if (bytes > std::size(buffer))
                mEsm->seekg(getFileOffset() + bytes);
            else
                mEsm->read(buffer, bytes);
        }

        /// Used for error handling
        [[noreturn]] void fail(const std::string& msg);

        /// Sets font encoder for ESM strings
        void setEncoder(ToUTF8::Utf8Encoder* encoder) { mEncoder = encoder; }

        /// Get record flags of last record
        unsigned int getRecordFlags() { return mRecordFlags; }

        size_t getFileSize() const { return mFileSize; }

    private:
        [[noreturn]] void reportSubSizeMismatch(size_t want, size_t got)
        {
            fail("record size mismatch, requested " + std::to_string(want) + ", got " + std::to_string(got));
        }

        void clearCtx();

        RefId getRefIdImpl(std::size_t size);

        std::unique_ptr<std::istream> mEsm;

        ESM_Context mCtx;

        unsigned int mRecordFlags;

        // Special file signifier (see SpecialFile enum above)

        // Buffer for ESM strings
        std::vector<char> mBuffer;

        Header mHeader;

        ToUTF8::Utf8Encoder* mEncoder;

        size_t mFileSize;
    };
}
#endif
