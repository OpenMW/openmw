/*
  Copyright (C) 2015-2016, 2018, 2020-2021 cc9cii

  This software is provided 'as-is', without any express or implied
  warranty.  In no event will the authors be held liable for any damages
  arising from the use of this software.

  Permission is granted to anyone to use this software for any purpose,
  including commercial applications, and to alter it and redistribute it
  freely, subject to the following restrictions:

  1. The origin of this software must not be misrepresented; you must not
     claim that you wrote the original software. If you use this software
     in a product, an acknowledgment in the product documentation would be
     appreciated but is not required.
  2. Altered source versions must be plainly marked as such, and must not be
     misrepresented as being the original software.
  3. This notice may not be removed or altered from any source distribution.

  cc9cii cc9c@iinet.net.au

*/
#ifndef ESM4_READER_H
#define ESM4_READER_H

#include <cstddef>
#include <filesystem>
#include <istream>
#include <map>
#include <memory>
#include <unordered_map>

#include "cellgrid.hpp"
#include "common.hpp"
#include "loadtes4.hpp"

#include <components/esm/formid.hpp>
#include <components/files/istreamptr.hpp>

namespace ToUTF8
{
    class StatelessUtf8Encoder;
}

namespace VFS
{
    class Manager;
}

namespace ESM4
{
#pragma pack(push, 1)
    // NOTE: the label field of a group is not reliable (http://www.uesp.net/wiki/Tes4Mod:Mod_File_Format)
    union GroupLabel
    {
        std::uint32_t value; // formId, blockNo or raw int representation of type
        char recordType[4]; // record type in ascii
        std::int16_t grid[2]; // grid y, x (note the reverse order)
    };

    struct GroupTypeHeader
    {
        std::uint32_t typeId;
        std::uint32_t groupSize; // includes the 24 bytes (20 for TES4) of header (i.e. this struct)
        GroupLabel label; // format based on type
        std::int32_t type;
        std::uint16_t stamp; // & 0xff for day, & 0xff00 for months since Dec 2002 (i.e. 1 = Jan 2003)
        std::uint16_t unknown;
        std::uint16_t version; // not in TES4
        std::uint16_t unknown2; // not in TES4
    };

    struct RecordTypeHeader
    {
        std::uint32_t typeId;
        std::uint32_t dataSize; // does *not* include 24 bytes (20 for TES4) of header
        std::uint32_t flags;
        ESM::FormId32 id;
        std::uint32_t revision;
        std::uint16_t version; // not in TES4
        std::uint16_t unknown; // not in TES4

        ESM::FormId getFormId() const { return ESM::FormId::fromUint32(id); }
    };

    union RecordHeader
    {
        GroupTypeHeader group;
        RecordTypeHeader record;
    };

    struct SubRecordHeader
    {
        std::uint32_t typeId;
        std::uint16_t dataSize;
    };
#pragma pack(pop)

    //                                                   bytes read from group, updated by
    //                                                   getRecordHeader() in advance
    //                                                     |
    //                                                     v
    typedef std::vector<std::pair<ESM4::GroupTypeHeader, std::uint32_t>> GroupStack;

    struct ReaderContext
    {
        std::filesystem::path filename; // in case we need to reopen to restore the context
        std::uint32_t modIndex; // the sequential position of this file in the load order:
        //  0x00 reserved, 0xFF in-game (see notes below)

        // position in the vector = mod index of master files above
        // value = adjusted mod index based on all the files loaded so far
        std::vector<std::uint32_t> parentFileIndices;

        std::size_t recHeaderSize; // normally should be already set correctly, but just in
        //  case the file was re-opened.  default = TES5 size,
        //  can be reduced for TES4 by setRecHeaderSize()

        std::streampos filePos; // assume that the record header will be re-read once
        //  the context is restored

        // for keeping track of things
        std::size_t fileRead; // number of bytes read, incl. the current record

        GroupStack groupStack; // keep track of bytes left to find when a group is done
        RecordHeader recordHeader; // header of the current record or group being processed
        SubRecordHeader subRecordHeader; // header of the current sub record being processed
        std::uint32_t recordRead; // bytes read from the sub records, incl. the current one

        ESM::FormId currWorld; // formId of current world - for grouping CELL records
        ESM::FormId currCell; // formId of current cell
        // FIXME: try to get rid of these two members, seem like massive hacks
        CellGrid currCellGrid; // TODO: should keep a map of cell formids
        bool cellGridValid;

        ReaderContext();
    };

    enum class LocalizedStringType
    {
        Strings,
        ILStrings,
        DLStrings,
    };

    class Reader
    {
        VFS::Manager const* mVFS;
        Header mHeader; // ESM4 header

        ReaderContext mCtx;

        const ToUTF8::StatelessUtf8Encoder* mEncoder;

        std::size_t mFileSize;

        Files::IStreamPtr mStream;
        Files::IStreamPtr mSavedStream; // mStream is saved here while using deflated memory stream

        Files::IStreamPtr mStrings;
        Files::IStreamPtr mILStrings;
        Files::IStreamPtr mDLStrings;

        std::unordered_map<ESM::FormId, std::string> mLStringIndex;

        std::vector<Reader*>* mGlobalReaderList = nullptr;

        bool mIgnoreMissingLocalizedStrings = false;

        void buildLStringIndex(LocalizedStringType stringType, const std::u8string& prefix);

        void buildLStringIndex(LocalizedStringType stringType, std::istream& stream);

        std::string readLocalizedString(LocalizedStringType type, std::istream& stream);

        inline bool hasLocalizedStrings() const { return (mHeader.mFlags & Rec_Localized) != 0; }

        void getLocalizedStringImpl(const ESM::FormId stringId, std::string& str);

        // Close the file, resets all information.
        // After calling close() the structure may be reused to load a new file.
        // void close();

        // Raw opening. Opens the file and sets everything up but doesn't parse the header.
        void openRaw(Files::IStreamPtr&& stream, const std::filesystem::path& filename);

        // Load ES file from a new stream, parses the header.
        // Closes the currently open file first, if any.
        void open(Files::IStreamPtr&& stream, const std::filesystem::path& filename);

        Reader() = default;

        bool getStringImpl(std::string& str, std::size_t size, std::istream& stream, bool hasNull = false);

    public:
        Reader(Files::IStreamPtr&& esmStream, const std::filesystem::path& filename, VFS::Manager const* vfs,
            const ToUTF8::StatelessUtf8Encoder* encoder, bool ignoreMissingLocalizedStrings = false);

        ~Reader();

        void open(const std::filesystem::path& filename);

        void close();

        inline bool isEsm4() const { return true; }

        const std::vector<ESM::MasterData>& getGameFiles() const { return mHeader.mMaster; }

        inline int getRecordCount() const { return mHeader.mData.records; }
        inline const std::string getAuthor() const { return mHeader.mAuthor; }
        inline int getFormat() const { return 0; } // prob. not relevant for ESM4
        inline const std::string getDesc() const { return mHeader.mDesc; }

        inline std::filesystem::path getFileName() const { return mCtx.filename; } // not used

        inline bool hasMoreRecs() const { return (mFileSize - mCtx.fileRead) > 0; }

        // Methods added for updating loading progress bars
        inline std::size_t getFileSize() const { return mFileSize; }
        inline std::size_t getFileOffset() const { return mStream->tellg(); }

        // Methods added for saving/restoring context
        ReaderContext getContext(); // WARN: must be called immediately after reading the record header

        bool restoreContext(const ReaderContext& ctx); // returns the result of re-reading the header

        template <typename T>
        inline void get(T& t)
        {
            mStream->read((char*)&t, sizeof(T));
        }

        // Use getFormId instead
        void get(ESM::FormId& value) = delete;

        template <typename T>
        bool getExact(T& t)
        {
            mStream->read((char*)&t, sizeof(T));
            return mStream->gcount() == sizeof(T); // FIXME: try/catch block needed?
        }

        // for arrays
        inline bool get(void* p, std::size_t size)
        {
            mStream->read((char*)p, size);
            return mStream->gcount() == (std::streamsize)size; // FIXME: try/catch block needed?
        }

        // NOTE: must be called before calling getRecordHeader()
        void setRecHeaderSize(const std::size_t size);

        inline unsigned int esmVersion() const { return mHeader.mData.version.ui; }
        inline float esmVersionF() const { return mHeader.mData.version.f; }
        inline unsigned int numRecords() const { return mHeader.mData.records; }

        inline bool hasFormVersion() const { return mCtx.recHeaderSize == sizeof(RecordHeader); }
        inline unsigned int formVersion() const { return mCtx.recordHeader.record.version; }

        void buildLStringIndex();
        void getLocalizedString(std::string& str);

        // Read 24 bytes of header. The caller can then decide whether to process or skip the data.
        bool getRecordHeader();

        inline const RecordHeader& hdr() const { return mCtx.recordHeader; }

        const GroupTypeHeader& grp(std::size_t pos = 0) const;

        // The object setting up this reader needs to supply the file's load order index
        // so that the formId's in this file can be adjusted with the file (i.e. mod) index.
        void setModIndex(std::uint32_t index) { mCtx.modIndex = index; }
        void updateModIndices(const std::map<std::string, int>& fileToModIndex);

        // Maybe should throw an exception if called when not valid?
        const CellGrid& currCellGrid() const;

        inline bool hasCellGrid() const { return mCtx.cellGridValid; }

        // This is set while loading a CELL record (XCLC sub record) and invalidated
        // each time loading a CELL (see clearCellGrid())
        inline void setCurrCellGrid(const CellGrid& currCell)
        {
            mCtx.cellGridValid = true;
            mCtx.currCellGrid = currCell;
        }

        // FIXME: This is called each time a new CELL record is read.  Rather than calling this
        // methos explicitly, mCellGridValid should be set automatically somehow.
        //
        // Cell 2c143 is loaded immedicatly after 1bdb1 and can mistakely appear to have grid 0, 1.
        inline void clearCellGrid() { mCtx.cellGridValid = false; }

        // Should be set at the beginning of a CELL load
        inline void setCurrCell(ESM::FormId formId) { mCtx.currCell = formId; }

        inline ESM::FormId currCell() const { return mCtx.currCell; }

        // Should be set at the beginning of a WRLD load
        inline void setCurrWorld(ESM::FormId formId) { mCtx.currWorld = formId; }

        inline ESM::FormId currWorld() const { return mCtx.currWorld; }

        // Get the data part of a record
        // Note: assumes the header was read correctly and nothing else was read
        void getRecordData(bool dump = false);

        // Skip the data part of a record
        // Note: assumes the header was read correctly (partial skip is allowed)
        void skipRecordData();

        // Skip the remaining part of the group
        // Note: assumes the header was read correctly and group was pushed onto the stack
        void skipGroupData();

        // Skip the group without pushing onto the stack
        // Note: assumes the header was read correctly and group was not pushed onto the stack
        // (expected to be used during development only while some groups are not yet supported)
        void skipGroup();

        // Read 6 bytes of header. The caller can then decide whether to process or skip the data.
        bool getSubRecordHeader();

        // Manally update (i.e. increase) the bytes read after SUB_XXXX
        inline void updateRecordRead(std::uint32_t subSize) { mCtx.recordRead += subSize; }

        inline const SubRecordHeader& subRecordHeader() const { return mCtx.subRecordHeader; }

        // Skip the data part of a subrecord
        // Note: assumes the header was read correctly and nothing else was read
        void skipSubRecordData();

        // Special for a subrecord following a XXXX subrecord
        void skipSubRecordData(std::uint32_t size);

        // Get a subrecord of a particular type and data type
        template <typename T>
        bool getSubRecord(const std::uint32_t type, T& t)
        {
            ESM4::SubRecordHeader hdr;
            if (!getExact(hdr) || (hdr.typeId != type) || (hdr.dataSize != sizeof(T)))
                return false;

            return get(t);
        }

        // ModIndex adjusted formId according to master file dependencies
        void adjustFormId(ESM::FormId& id) const;

        // Temporary. Doesn't support mod index > 255
        void adjustFormId(ESM::FormId32& id) const;

        bool getFormId(ESM::FormId& id);
        ESM::FormId getFormIdFromHeader() const;

        void adjustGRUPFormId();

        // Note: uses the string size from the subrecord header rather than checking null termination
        bool getZString(std::string& str) { return getStringImpl(str, mCtx.subRecordHeader.dataSize, *mStream, true); }
        bool getString(std::string& str) { return getStringImpl(str, mCtx.subRecordHeader.dataSize, *mStream); }

        bool getZeroTerminatedStringArray(std::vector<std::string>& values);

        void enterGroup();
        void exitGroupCheck();

        // for debugging only
        size_t stackSize() const { return mCtx.groupStack.size(); }

        // Used for error handling
        [[noreturn]] void fail(const std::string& msg);

        void setGlobalReaderList(std::vector<Reader*>* list) { mGlobalReaderList = list; }

        std::vector<Reader*>* getGlobalReaderList() { return mGlobalReaderList; }
    };

    // For pretty printing GroupHeader labels
    std::string printLabel(const GroupLabel& label, const std::uint32_t type);
}

#endif // ESM4_READER_H
