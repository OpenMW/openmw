#include "esm4reader.hpp"

ESM::ESM4Reader::ESM4Reader(bool oldHeader)
{
    // TES4 header size is 4 bytes smaller than TES5 header
    mReader.setRecHeaderSize(oldHeader ? sizeof(ESM4::RecordHeader)-4 : sizeof(ESM4::RecordHeader));
}

ESM::ESM4Reader::~ESM4Reader()
{
}

void ESM::ESM4Reader::openTes4File(const std::string &name)
{
    mCtx.filename = name;
    // WARNING: may throw
    mCtx.leftFile = mReader.openTes4File(name);
    mReader.registerForUpdates(this); // for updating mCtx.leftFile

    mReader.getRecordHeader();
    if (mReader.hdr().record.typeId == ESM4::REC_TES4)
    {
        mReader.loadHeader();
        mCtx.leftFile -= mReader.hdr().record.dataSize;

        // Hack: copy over values to TES3 header for getVer() and getRecordCount() to work
        mHeader.mData.version = mReader.esmVersion();
        mHeader.mData.records = mReader.numRecords();

        mReader.buildLStringIndex(); // for localised strings in Skyrim
    }
    else
        fail("Unknown file format");
}

ESM4::ReaderContext ESM::ESM4Reader::getESM4Context()
{
    return mReader.getContext();
}

void ESM::ESM4Reader::restoreESM4Context(const ESM4::ReaderContext& ctx)
{
    // Reopen the file if necessary
    if (mCtx.filename != ctx.filename)
        openTes4File(ctx.filename);

    // mCtx.leftFile is the only thing used in the old context.  Strictly speaking, updating it
    // with the correct value is not really necessary since we're not going to load the rest of
    // the file (most likely to load a CELL or LAND then be done with it).
    mCtx.leftFile = mReader.getFileSize() - mReader.getFileOffset();

    // restore group stack, load the header, etc.
    mReader.restoreContext(ctx);
}

void ESM::ESM4Reader::restoreCellChildrenContext(const ESM4::ReaderContext& ctx)
{
    // Reopen the file if necessary
    if (mCtx.filename != ctx.filename)
        openTes4File(ctx.filename);

    mReader.restoreContext(ctx); // restore group stack, load the CELL header, etc.
    if (mReader.hdr().record.typeId != ESM4::REC_CELL) // FIXME: testing only
        fail("Restore Cell Children failed");
    mReader.skipRecordData();    // skip the CELL record

    mReader.getRecordHeader();   // load the header for cell child group (hopefully)
    // this is a hack to load only the cell child group...
    if (mReader.hdr().group.typeId == ESM4::REC_GRUP && mReader.hdr().group.type == ESM4::Grp_CellChild)
    {
        mCtx.leftFile = mReader.hdr().group.groupSize - ctx.recHeaderSize;
        return;
    }

    // But some cells may have no child groups...
    // Suspect "ICMarketDistrict" 7 18 is one, followed by cell record 00165F2C "ICMarketDistrict" 6 17
    if (mReader.hdr().group.typeId != ESM4::REC_GRUP && mReader.hdr().record.typeId == ESM4::REC_CELL)
    {
        mCtx.leftFile = 0;
        return;
    }

    // Maybe the group is completed
    // See "ICMarketDistrict" 9 15 which is followed by a exterior sub-cell block
    ESM4::ReaderContext tempCtx = mReader.getContext();
    if (!tempCtx.groupStack.empty() && tempCtx.groupStack.back().second == 0)
    {
        mCtx.leftFile = 0;
        return;
    }
    else
        fail("Restore Cell Children failed");

}
