/*
  Copyright (C) 2015-2016, 2018 cc9cii

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

#include <vector>
#include <map>
#include <cstddef>

#include <boost/scoped_array.hpp>

#include <OgreDataStream.h>

#include "common.hpp"
#include "tes4.hpp"

namespace ESM4
{
    class ReaderObserver
    {
    public:
        ReaderObserver() {}
        virtual ~ReaderObserver() {}

        virtual void update(std::size_t size) = 0;
    };

    typedef std::vector<std::pair<ESM4::GroupTypeHeader, std::uint32_t> > GroupStack;

    struct ReaderContext
    {
        std::string     filename;         // from openTes4File()
        std::uint32_t   modIndex;         // the sequential position of this file in the load order:
                                          //  0x00 reserved, 0xFF in-game (see notes below)

        GroupStack      groupStack;       // keep track of bytes left to find when a group is done

        FormId          currWorld;        // formId of current world - for grouping CELL records
        FormId          currCell;         // formId of current cell

        std::size_t     recHeaderSize;    // normally should be already set correctly, but just in
                                          //  case the file was re-opened.  default = TES5 size,
                                          //  can be reduced for TES4 by setRecHeaderSize()

        std::size_t     filePos;          // assume that the record header will be re-read once
                                          //  the context is restored.
    };

    class Reader
    {
        ReaderObserver *mObserver;        // observer for tracking bytes read

        Header          mHeader;          // ESM header // FIXME
        RecordHeader    mRecordHeader;    // header of the current record or group being processed
        SubRecordHeader mSubRecordHeader; // header of the current sub record being processed
        std::size_t     mRecordRemaining; // number of bytes to be read by sub records following current

        // FIXME: try to get rid of these two members, seem like massive hacks
        CellGrid        mCurrCellGrid;    // TODO: should keep a map of cell formids
        bool            mCellGridValid;

        ReaderContext   mCtx;

        // Use scoped arrays to avoid memory leak due to exceptions, etc.
        // TODO: try fixed size buffers on the stack for both buffers (may be faster)
        boost::scoped_array<unsigned char> mInBuf;
        boost::scoped_array<unsigned char> mDataBuf;

        Ogre::DataStreamPtr mStream;
        Ogre::DataStreamPtr mSavedStream; // mStream is saved here while using deflated memory stream

        Ogre::DataStreamPtr mStrings;
        Ogre::DataStreamPtr mILStrings;
        Ogre::DataStreamPtr mDLStrings;

        enum LocalizedStringType
        {
            Type_Strings   = 0,
            Type_ILStrings = 1,
            Type_DLStrings = 2
        };

        struct LStringOffset
        {
            LocalizedStringType type;
            std::uint32_t offset;
        };
        std::map<FormId, LStringOffset> mLStringIndex;

        void getRecordDataPostActions(); // housekeeping actions before processing the next record
        void buildLStringIndex(const std::string& stringFile, LocalizedStringType stringType);

    public:

        Reader();
        ~Reader();

        // Methods added for updating loading progress bars
        inline std::size_t getFileSize() const { return mStream->size(); }
        inline std::size_t getFileOffset() const { return mStream->tell(); }

        // Methods added for saving/restoring context
        ReaderContext getContext();
        bool restoreContext(const ReaderContext& ctx); // returns the result of re-reading the header
        bool skipNextGroupCellChild(); // returns true if skipped

        std::size_t openTes4File(const std::string& name);

        // NOTE: must be called before calling getRecordHeader()
        void setRecHeaderSize(const std::size_t size);

        inline void loadHeader() { mHeader.load(*this); }
        inline unsigned int esmVersion() const { return mHeader.mData.version.ui; }
        inline unsigned int numRecords() const { return mHeader.mData.records; }

        void buildLStringIndex();
        inline bool hasLocalizedStrings() const { return (mHeader.mFlags & Rec_Localized) != 0; }
        void getLocalizedString(std::string& str); // convenience method for below
        void getLocalizedString(const FormId stringId, std::string& str);

        // Read 24 bytes of header. The caller can then decide whether to process or skip the data.
        bool getRecordHeader();

        inline const RecordHeader& hdr() const { return mRecordHeader; }

        const GroupTypeHeader& grp(std::size_t pos = 0) const;

        // The object setting up this reader needs to supply the file's load order index
        // so that the formId's in this file can be adjusted with the file (i.e. mod) index.
        void setModIndex(int index) { mCtx.modIndex = (index << 24) & 0xff000000; }
        void updateModIndicies(const std::vector<std::string>& files);

        // Maybe should throw an exception if called when not valid?
        const CellGrid& currCellGrid() const;

        inline const bool hasCellGrid() const { return mCellGridValid; }

        // This is set while loading a CELL record (XCLC sub record) and invalidated
        // each time loading a CELL (see clearCellGrid())
        inline void setCurrCellGrid(const CellGrid& currCell) {
            mCellGridValid = true;
            mCurrCellGrid = currCell;
        }

        // FIXME: This is called each time a new CELL record is read.  Rather than calling this
        // methos explicitly, mCellGridValid should be set automatically somehow.
        //
        // Cell 2c143 is loaded immedicatly after 1bdb1 and can mistakely appear to have grid 0, 1.
        inline void clearCellGrid() { mCellGridValid = false; }

        // Should be set at the beginning of a CELL load
        inline void setCurrCell(FormId formId) { mCtx.currCell = formId; }

        inline FormId currCell() const { return mCtx.currCell; }

        // Should be set at the beginning of a WRLD load
        inline void setCurrWorld(FormId formId) { mCtx.currWorld = formId; }

        inline FormId currWorld() const { return mCtx.currWorld; }

        // Get the data part of a record
        // Note: assumes the header was read correctly and nothing else was read
        void getRecordData();

        // Skip the data part of a record
        // Note: assumes the header was read correctly and nothing else was read
        void skipRecordData();

        // Skip the rest of the group
        // Note: assumes the header was read correctly and nothing else was read
        void skipGroup();

        // Read 6 bytes of header. The caller can then decide whether to process or skip the data.
        bool getSubRecordHeader();

        // Manally update (i.e. reduce) the bytes remaining to be read after SUB_XXXX
        inline void updateRecordRemaining(std::uint32_t subSize) { mRecordRemaining -= subSize; }

        inline const SubRecordHeader& subRecordHeader() const { return mSubRecordHeader; }

        // Skip the data part of a subrecord
        // Note: assumes the header was read correctly and nothing else was read
        void skipSubRecordData();

        // Special for a subrecord following a XXXX subrecord
        void skipSubRecordData(std::uint32_t size);

        // Get a subrecord of a particular type and data type
        template<typename T>
        bool getSubRecord(const ESM4::SubRecordTypes type, T& t)
        {
            ESM4::SubRecordHeader hdr;
            if (!get(hdr) || (hdr.typeId != type) || (hdr.dataSize != sizeof(T)))
                return false;

            return get(t);
        }

        template<typename T>
        inline bool get(T& t) {
            return mStream->read(&t, sizeof(T)) == sizeof(T); // FIXME: try/catch block needed?
        }

        // for arrays
        inline bool get(void* p, std::size_t size) {
            return mStream->read(p, size) == size;            // FIXME: try/catch block needed?
        }

        // ModIndex adjusted formId according to master file dependencies
        void adjustFormId(FormId& id);

        bool getFormId(FormId& id);

        void adjustGRUPFormId();

        // Note: does not convert to UTF8
        // Note: assumes string size from the subrecord header
        bool getZString(std::string& str);
        bool getZString(std::string& str, Ogre::DataStreamPtr fileStream);

        void checkGroupStatus();

        void saveGroupStatus();

        void registerForUpdates(ReaderObserver *observer);

        // for debugging only
        size_t stackSize() const { return mCtx.groupStack.size(); }
    };

    // An idea on extending the 254 mods limit
    // ---------------------------------------
    //
    // By using a 64bit formid internally it should be possible to extend the limit.  However
    // saved games won't be compatible.
    //
    // One or two digits can be used, which will allow 4096-2=4094 or 65535-2=65533 mods.
    // With the remaining digits one can be used as a game index (e.g. TES3=0, TES4=1, etc).
    //
    // The remaining bits might still be useful for indicating something else about the object.
    //
    //       game index
    //         |
    //         | mod index extend to 4 digits (or 3 digits?)
    //         | +---+
    //         | |   |
    //         v v   v
    // 0xfffff f ff ff ffffff
    //              ^^ ^    ^
    //              || |    |
    //              || +----+
    //              || 6 digit obj index
    //              ++
    //            2 digit mod index
    //
}

#endif // ESM4_READER_H
