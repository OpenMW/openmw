#include "esmreader.hpp"
#include <stdexcept>

#include "../files/constrainedfiledatastream.hpp"

namespace ESM
{

using namespace Misc;

ESM_Context ESMReader::getContext()
{
    // Update the file position before returning
    mCtx.filePos = mEsm->tell();
    return mCtx;
}

ESMReader::ESMReader(void):
    mBuffer(50*1024)
{
}

void ESMReader::restoreContext(const ESM_Context &rc)
{
    // Reopen the file if necessary
    if (mCtx.filename != rc.filename)
        openRaw(rc.filename);

    // Copy the data
    mCtx = rc;

    // Make sure we seek to the right place
    mEsm->seek(mCtx.filePos);
}

void ESMReader::close()
{
    mEsm.setNull();
    mCtx.filename.clear();
    mCtx.leftFile = 0;
    mCtx.leftRec = 0;
    mCtx.leftSub = 0;
    mCtx.subCached = false;
    mCtx.recName.val = 0;
    mCtx.subName.val = 0;
}

void ESMReader::openRaw(Ogre::DataStreamPtr _esm, const std::string &name)
{
    close();
    mEsm = _esm;
    mCtx.filename = name;
    mCtx.leftFile = mEsm->size();

    // Flag certain files for special treatment, based on the file
    // name.
    const char *cstr = mCtx.filename.c_str();
    if (iends(cstr, "Morrowind.esm"))
        mSpf = SF_Morrowind;
    else if (iends(cstr, "Tribunal.esm"))
        mSpf = SF_Tribunal;
    else if (iends(cstr, "Bloodmoon.esm"))
        mSpf = SF_Bloodmoon;
    else
        mSpf = SF_Other;
}

void ESMReader::open(Ogre::DataStreamPtr _esm, const std::string &name)
{
    openRaw(_esm, name);

    if (getRecName() != "TES3")
        fail("Not a valid Morrowind file");

    getRecHeader();

    // Get the header
    getHNT(mCtx.header, "HEDR", 300);

    if (mCtx.header.version != VER_12 && mCtx.header.version != VER_13)
        fail("Unsupported file format version");

    while (isNextSub("MAST"))
    {
        MasterData m;
        m.name = getHString();
        m.size = getHNLong("DATA");
        mMasters.push_back(m);
    }

    if (mCtx.header.type == FT_ESS)
    {
        // Savegame-related data

        // Player position etc
        getHNT(mSaveData, "GMDT", 124);

        /* Image properties, five ints. Is always:
         Red-mask:   0xff0000
         Blue-mask:  0x00ff00
         Green-mask: 0x0000ff
         Alpha-mask: 0x000000
         Bpp:        32
         */
        getSubNameIs("SCRD");
        skipHSubSize(20);

        /* Savegame screenshot:
         128x128 pixels * 4 bytes per pixel
         */
        getSubNameIs("SCRS");
        skipHSubSize(65536);
    }
}

void ESMReader::open(const std::string &file)
{
    open (openConstrainedFileDataStream (file.c_str ()), file);
}

void ESMReader::openRaw(const std::string &file)
{
    openRaw (openConstrainedFileDataStream (file.c_str ()), file);
}

int64_t ESMReader::getHNLong(const char *name)
{
    int64_t val;
    getHNT(val, name);
    return val;
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
    if (mCtx.leftSub == 0)
    {
        // Skip the following zero byte
        mCtx.leftRec--;
        char c;
        mEsm->read(&c, 1);
        return "";
    }

    return getString(mCtx.leftSub);
}

void ESMReader::getHExact(void*p, int size)
{
    getSubHeader();
    if (size != static_cast<int> (mCtx.leftSub))
        fail("getHExact() size mismatch");
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
        fail(
                "Expected subrecord " + std::string(name) + " but got "
                        + mCtx.subName.toString());
}

bool ESMReader::isNextSub(const char* name)
{
    if (!mCtx.leftRec)
        return false;

    getSubName();

    // If the name didn't match, then mark the it as 'cached' so it's
    // available for the next call to getSubName.
    mCtx.subCached = (mCtx.subName != name);

    // If subCached is false, then subName == name.
    return !mCtx.subCached;
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
    mEsm->read(mCtx.subName.name, 4);
    mCtx.leftRec -= 4;
}

bool ESMReader::isEmptyOrGetName()
{
    if (mCtx.leftRec)
    {
        mEsm->read(mCtx.subName.name, 4);
        mCtx.leftRec -= 4;
        return false;
    }
    return true;
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
        fail("skipHSubSize() mismatch");
}

void ESMReader::getSubHeader()
{
    if (mCtx.leftRec < 4)
        fail("End of record while reading sub-record header");

    // Get subrecord size
    getT(mCtx.leftSub);

    // Adjust number of record bytes left
    mCtx.leftRec -= mCtx.leftSub + 4;
}

void ESMReader::getSubHeaderIs(int size)
{
    getSubHeader();
    if (size != static_cast<int> (mCtx.leftSub))
        fail("getSubHeaderIs(): Sub header mismatch");
}

NAME ESMReader::getRecName()
{
    if (!hasMoreRecs())
        fail("No more records, getRecName() failed");
    getName(mCtx.recName);
    mCtx.leftFile -= 4;

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
}

void ESMReader::skipHRecord()
{
    if (!mCtx.leftFile)
        return;
    getRecHeader();
    skipRecord();
}

void ESMReader::getRecHeader(uint32_t &flags)
{
    // General error checking
    if (mCtx.leftFile < 12)
        fail("End of file while reading record header");
    if (mCtx.leftRec)
        fail("Previous record contains unread bytes");

    getUint(mCtx.leftRec);
    getUint(flags);// This header entry is always zero
    getUint(flags);
    mCtx.leftFile -= 12;

    // Check that sizes add up
    if (mCtx.leftFile < mCtx.leftRec)
        fail("Record size is larger than rest of file");

    // Adjust number of bytes mCtx.left in file
    mCtx.leftFile -= mCtx.leftRec;
}

/*************************************************************************
 *
 *  Lowest level data reading and misc methods
 *
 *************************************************************************/

void ESMReader::getExact(void*x, int size)
{
    int t = mEsm->read(x, size);
    if (t != size)
        fail("Read error");
}

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
    char *ptr = &mBuffer[0];
    getExact(ptr, size);

    // Convert to UTF8 and return
    return mEncoder->getUtf8(ptr, size);
}

void ESMReader::fail(const std::string &msg)
{
    using namespace std;

    stringstream ss;

    ss << "ESM Error: " << msg;
    ss << "\n  File: " << mCtx.filename;
    ss << "\n  Record: " << mCtx.recName.toString();
    ss << "\n  Subrecord: " << mCtx.subName.toString();
    if (!mEsm.isNull())
        ss << "\n  Offset: 0x" << hex << mEsm->tell();
    throw std::runtime_error(ss.str());
}

void ESMReader::setEncoder(ToUTF8::Utf8Encoder* encoder)
{
    mEncoder = encoder;
}

}
