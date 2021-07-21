#include "esmreader.hpp"

#include <stdexcept>

namespace ESM
{

using namespace Misc;

ESM_Context ESMReader::getContext()
{
    // Update the file position before returning
    mCtx.filePos = mEsm->tellg();
    return mCtx;
}

ESMReader::ESMReader()
    : mRecordFlags(0)
    , mBuffer(50*1024)
    , mGlobalReaderList(nullptr)
    , mEncoder(nullptr)
    , mFileSize(0)
{
    clearCtx();
}

void ESMReader::restoreContext(const ESM_Context &rc)
{
    // Reopen the file if necessary
    if (mCtx.filename != rc.filename)
        openRaw(rc.filename);

    // Copy the data
    mCtx = rc;

    // Make sure we seek to the right place
    mEsm->seekg(mCtx.filePos);
}

void ESMReader::close()
{
    mEsm.reset();
    clearCtx();
    mHeader.blank();
}

void ESMReader::clearCtx() 
{
   mCtx.filename.clear();
   mCtx.leftFile = 0;
   mCtx.leftRec = 0;
   mCtx.leftSub = 0;
   mCtx.subCached = false;
   mCtx.recName.clear();
   mCtx.subName.clear();
}

void ESMReader::openRaw(Files::IStreamPtr _esm, const std::string& name)
{
    close();
    mEsm = _esm;
    mCtx.filename = name;
    mEsm->seekg(0, mEsm->end);
    mCtx.leftFile = mFileSize = mEsm->tellg();
    mEsm->seekg(0, mEsm->beg);
}

void ESMReader::openRaw(const std::string& filename)
{
    openRaw(Files::openConstrainedFileStream(filename.c_str()), filename);
}

void ESMReader::open(Files::IStreamPtr _esm, const std::string &name)
{
    openRaw(_esm, name);

    if (getRecName() != "TES3")
        fail("Not a valid Morrowind file");

    getRecHeader();

    mHeader.load (*this);
}

void ESMReader::open(const std::string &file)
{
    open (Files::openConstrainedFileStream (file.c_str ()), file);
}

std::string ESMReader::getHNOString(const char* name)
{
    if (isNextSub(name))
        return getHString();
    return "";
}

std::string ESMReader::getHNString(const char* name)
{
    getSubNameIs(name);
    return getHString();
}

std::string ESMReader::getHString()
{
    getSubHeader();

    // Hack to make MultiMark.esp load. Zero-length strings do not
    // occur in any of the official mods, but MultiMark makes use of
    // them. For some reason, they break the rules, and contain a byte
    // (value 0) even if the header says there is no data. If
    // Morrowind accepts it, so should we.
    if (mCtx.leftSub == 0 && hasMoreSubs() && !mEsm->peek())
    {
        // Skip the following zero byte
        mCtx.leftRec--;
        char c;
        getT(c);
        return std::string();
    }

    return getString(mCtx.leftSub);
}

void ESMReader::getHExact(void*p, int size)
{
    getSubHeader();
    if (size != static_cast<int> (mCtx.leftSub))
        reportSubSizeMismatch(size, mCtx.leftSub);
    getExact(p, size);
}

// Read the given number of bytes from a named subrecord
void ESMReader::getHNExact(void*p, int size, const char* name)
{
    getSubNameIs(name);
    getHExact(p, size);
}

// Get the next subrecord name and check if it matches the parameter
void ESMReader::getSubNameIs(const char* name)
{
    getSubName();
    if (mCtx.subName != name)
        fail("Expected subrecord " + std::string(name) + " but got " + mCtx.subName.toString());
}

bool ESMReader::isNextSub(const char* name)
{
    if (!hasMoreSubs())
        return false;

    getSubName();

    // If the name didn't match, then mark the it as 'cached' so it's
    // available for the next call to getSubName.
    mCtx.subCached = (mCtx.subName != name);

    // If subCached is false, then subName == name.
    return !mCtx.subCached;
}

bool ESMReader::peekNextSub(const char *name)
{
    if (!hasMoreSubs())
        return false;

    getSubName();

    mCtx.subCached = true;
    return mCtx.subName == name;
}

// Read subrecord name. This gets called a LOT, so I've optimized it
// slightly.
void ESMReader::getSubName()
{
    // If the name has already been read, do nothing
    if (mCtx.subCached)
    {
        mCtx.subCached = false;
        return;
    }

    // reading the subrecord data anyway.
    const int subNameSize = static_cast<int>(mCtx.subName.data_size());
    getExact(mCtx.subName.rw_data(), subNameSize);
    mCtx.leftRec -= static_cast<uint32_t>(subNameSize);
}

void ESMReader::skipHSub()
{
    getSubHeader();
    skip(mCtx.leftSub);
}

void ESMReader::skipHSubSize(int size)
{
    skipHSub();
    if (static_cast<int> (mCtx.leftSub) != size)
        reportSubSizeMismatch(mCtx.leftSub, size);
}

void ESMReader::skipHSubUntil(const char *name)
{
    while (hasMoreSubs() && !isNextSub(name))
    {
        mCtx.subCached = false;
        skipHSub();
    }
    if (hasMoreSubs())
        mCtx.subCached = true;
}

void ESMReader::getSubHeader()
{
    if (mCtx.leftRec < sizeof(mCtx.leftSub))
        fail("End of record while reading sub-record header");

    // Get subrecord size
    getT(mCtx.leftSub);
    mCtx.leftRec -= sizeof(mCtx.leftSub);

    // Adjust number of record bytes left
    if (mCtx.leftRec < mCtx.leftSub)
        fail("Record size is larger than rest of file");
    mCtx.leftRec -= mCtx.leftSub;
}

NAME ESMReader::getRecName()
{
    if (!hasMoreRecs())
        fail("No more records, getRecName() failed");
    getName(mCtx.recName);
    mCtx.leftFile -= mCtx.recName.data_size();

    // Make sure we don't carry over any old cached subrecord
    // names. This can happen in some cases when we skip parts of a
    // record.
    mCtx.subCached = false;

    return mCtx.recName;
}

void ESMReader::skipRecord()
{
    skip(mCtx.leftRec);
    mCtx.leftRec = 0;
    mCtx.subCached = false;
}

void ESMReader::getRecHeader(uint32_t &flags)
{
    // General error checking
    if (mCtx.leftFile < 3 * sizeof(uint32_t))
        fail("End of file while reading record header");
    if (mCtx.leftRec)
        fail("Previous record contains unread bytes");

    getUint(mCtx.leftRec);
    getUint(flags);// This header entry is always zero
    getUint(flags);
    mCtx.leftFile -= 3 * sizeof(uint32_t);

    // Check that sizes add up
    if (mCtx.leftFile < mCtx.leftRec)
        reportSubSizeMismatch(mCtx.leftFile, mCtx.leftRec);

    // Adjust number of bytes mCtx.left in file
    mCtx.leftFile -= mCtx.leftRec;
}

/*************************************************************************
 *
 *  Lowest level data reading and misc methods
 *
 *************************************************************************/

std::string ESMReader::getString(int size)
{
    size_t s = size;
    if (mBuffer.size() <= s)
        // Add some extra padding to reduce the chance of having to resize
        // again later.
        mBuffer.resize(3*s);

    // And make sure the string is zero terminated
    mBuffer[s] = 0;

    // read ESM data
    char *ptr = mBuffer.data();
    getExact(ptr, size);

    size = static_cast<int>(strnlen(ptr, size));

    // Convert to UTF8 and return
    if (mEncoder)
        return mEncoder->getUtf8(ptr, size);

    return std::string (ptr, size);
}

[[noreturn]] void ESMReader::fail(const std::string &msg)
{
    std::stringstream ss;

    ss << "ESM Error: " << msg;
    ss << "\n  File: " << mCtx.filename;
    ss << "\n  Record: " << mCtx.recName.toString();
    ss << "\n  Subrecord: " << mCtx.subName.toString();
    if (mEsm.get())
        ss << "\n  Offset: 0x" << std::hex << mEsm->tellg();
    throw std::runtime_error(ss.str());
}

}
