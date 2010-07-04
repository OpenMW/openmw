#ifndef _ESM_READER_H
#define _ESM_READER_H

#include <string>
#include <libs/platform/stdint.h>
#include <string.h>
#include <assert.h>
#include <vector>
#include <sstream>
#include <iomanip>

#include <libs/mangle/stream/stream.hpp>
#include <libs/mangle/stream/servers/file_stream.hpp>
#include <libs/mangle/tools/str_exception.hpp>
#include <components/misc/stringops.hpp>

#ifdef __APPLE__
// need our own implementation of strnlen
static size_t strnlen(const char *s, size_t n)
{
  const char *p = (const char *)memchr(s, 0, n);
  return(p ? p-s : n);
}

#endif

namespace ESM {

enum Version
  {
    VER_12 = 0x3f99999a,
    VER_13 = 0x3fa66666
  };

enum FileType
  {
    FT_ESP = 0,       // Plugin
    FT_ESM = 1,       // Master
    FT_ESS = 32       // Savegame
  };

// Used to mark special files. The original ESM files are given
// special treatment in a few places, most noticably in loading and
// filtering out "dirtly" GMST entries correctly.
enum SpecialFile
  {
    SF_Other,
    SF_Morrowind,
    SF_Tribunal,
    SF_Bloodmoon
  };

/* A structure used for holding fixed-length strings. In the case of
   LEN=4, it can be more efficient to match the string as a 32 bit
   number, therefore the struct is implemented as a union with an int.
 */
template <int LEN>
union NAME_T
{
  char name[LEN];
  int32_t val;

  bool operator==(const char *str)
  {
    for(int i=0; i<LEN; i++)
      if(name[i] != str[i]) return false;
      else if(name[i] == 0) return true;
    return str[LEN] == 0;
  }
  bool operator!=(const char *str) { return !((*this)==str); }

  bool operator==(const std::string &str)
  {
    return (*this) == str.c_str();
  }
  bool operator!=(const std::string &str) { return !((*this)==str); }

  bool operator==(int v) { return v == val; }
  bool operator!=(int v) { return v != val; }

  std::string toString() const { return std::string(name, strnlen(name, LEN)); }
};

typedef NAME_T<4> NAME;
typedef NAME_T<32> NAME32;
typedef NAME_T<64> NAME64;
typedef NAME_T<256> NAME256;

#pragma pack(push)
#pragma pack(1)
/// File header data for all ES files
struct HEDRstruct
{
  /* File format version. This is actually a float, the supported
     versions are 1.2 and 1.3. These correspond to:
     1.2 = 0x3f99999a and 1.3 = 0x3fa66666
  */
  int version;
  int type;           // 0=esp, 1=esm, 32=ess
  NAME32 author;      // Author's name
  NAME256 desc;       // File description
  int records;        // Number of records? Not used.
};

// Defines another files (esm or esp) that this file depends upon.
struct MasterData
{
  std::string name;
  uint64_t size;
};

// Data that is only present in save game files
struct SaveData
{
  float pos[6];     // Player position and rotation
  NAME64 cell;      // Cell name
  float unk2;       // Unknown value - possibly game time?
  NAME32 player;    // Player name
};
#pragma pack(pop)


/* This struct defines a file 'context' which can be saved and later
   restored by an ESMReader instance. It will save the position within
   a file, and when restored will let you read from that position as
   if you never left it.
 */
struct ESM_Context
{
  std::string filename;
  uint32_t leftRec, leftSub;
  size_t leftFile;
  NAME recName, subName;
  HEDRstruct header;

  // True if subName has been read but not used.
  bool subCached;

  // File position. Only used for stored contexts, not regularly
  // updated within the reader itself.
  size_t filePos;
};

class ESMReader
{
public:

  /*************************************************************************
   *
   *  Public type definitions
   *
   *************************************************************************/

  typedef std::vector<MasterData> MasterList;

  /*************************************************************************
   *
   *  Information retrieval
   *
   *************************************************************************/

  int getVer() { return c.header.version; }
  float getFVer() { return *((float*)&c.header.version); }
  int getSpecial() { return spf; }
  const std::string getAuthor() { return c.header.author.toString(); }
  const std::string getDesc() { return c.header.desc.toString(); }
  const SaveData &getSaveData() { return saveData; }
  const MasterList &getMasters() { return masters; }
  const NAME &retSubName() { return c.subName; }
  uint32_t getSubSize() { return c.leftSub; }

  /*************************************************************************
   *
   *  Opening and closing
   *
   *************************************************************************/

  /** Save the current file position and information in a ESM_Context
      struct
   */
  ESM_Context getContext()
  {
    // Update the file position before returning
    c.filePos = esm->tell();
    return c;
  }

  /** Restore a previously saved context */
  void restoreContext(const ESM_Context &rc)
  {
    // Reopen the file if necessary
    if(c.filename != rc.filename)
      openRaw(rc.filename);

    // Copy the data
    c = rc;

    // Make sure we seek to the right place
    esm->seek(c.filePos);
  }

  /** Close the file, resets all information. After calling close()
      the structure may be reused to load a new file.
  */
  void close()
  {
    esm.reset();
    c.filename.clear();
    c.leftFile = 0;
    c.leftRec = 0;
    c.leftSub = 0;
    c.subCached = false;
    c.recName.val = 0;
    c.subName.val = 0;
  }

  /// Raw opening. Opens the file and sets everything up but doesn't
  /// parse the header.
  void openRaw(Mangle::Stream::StreamPtr _esm, const std::string &name)
  {  
    close();
    esm = _esm;
    c.filename = name;
    c.leftFile = esm->size();

    // Flag certain files for special treatment, based on the file
    // name.
    const char *cstr = c.filename.c_str();
    if(iends(cstr, "Morrowind.esm")) spf = SF_Morrowind;
    else if(iends(cstr, "Tribunal.esm")) spf = SF_Tribunal;
    else if(iends(cstr, "Bloodmoon.esm")) spf = SF_Bloodmoon;
    else spf = SF_Other;
  }

  /// Load ES file from a new stream, parses the header. Closes the
  /// currently open file first, if any.
  void open(Mangle::Stream::StreamPtr _esm, const std::string &name)
  {
    openRaw(_esm, name);

    if(getRecName() != "TES3")
      fail("Not a valid Morrowind file");

    getRecHeader();

    // Get the header
    getHNT(c.header, "HEDR", 300);

    if(c.header.version != VER_12 &&
       c.header.version != VER_13)
      fail("Unsupported file format version");

    while(isNextSub("MAST"))
      {
        MasterData m;
        m.name = getHString();
        m.size = getHNLong("DATA");
        masters.push_back(m);
      }

    if(c.header.type == FT_ESS)
      {
        // Savegame-related data

        // Player position etc
        getHNT(saveData, "GMDT", 124);

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

  void open(const std::string &file)
  {
    using namespace Mangle::Stream;
    open(StreamPtr(new FileStream(file)), file);
  }

  void openRaw(const std::string &file)
  {
    using namespace Mangle::Stream;
    openRaw(StreamPtr(new FileStream(file)), file);
  }

  /*************************************************************************
   *
   *  Medium-level reading shortcuts
   *
   *************************************************************************/

  // Read data of a given type, stored in a subrecord of a given name
  template <typename X>
  void getHNT(X &x, const char* name)
  {
    getSubNameIs(name);
    getHT(x);
  }

  // Optional version of getHNT
  template <typename X>
  void getHNOT(X &x, const char* name)
  {
    if(isNextSub(name))
      getHT(x);
  }

  // Version with extra size checking, to make sure the compiler
  // doesn't mess up our struct padding.
  template <typename X>
  void getHNT(X &x, const char* name, int size)
  {
    assert(sizeof(X) == size);
    getSubNameIs(name);
    getHT(x);
  }

  int64_t getHNLong(const char *name)
  {
    int64_t val;
    getHNT(val, name);
    return val;
  }

  // Get data of a given type/size, including subrecord header
  template <typename X>
  void getHT(X &x)
  {
    getSubHeader();
    if(c.leftSub != sizeof(X))
      fail("getHT(): subrecord size mismatch");
    getT(x);
  }

  // Version with extra size checking, to make sure the compiler
  // doesn't mess up our struct padding.
  template <typename X>
  void getHT(X &x, int size)
  {
    assert(sizeof(X) == size);
    getHT(x);
  }

  // Read a string by the given name if it is the next record.
  std::string getHNOString(const char* name)
  {
    if(isNextSub(name))
      return getHString();
    return "";
  }

  // Read a string with the given sub-record name
  std::string getHNString(const char* name)
  {
    getSubNameIs(name);
    return getHString();
  }

  // Read a string, including the sub-record header (but not the name)
  std::string getHString()
  {
    getSubHeader();

    // Hack to make MultiMark.esp load. Zero-length strings do not
    // occur in any of the official mods, but MultiMark makes use of
    // them. For some reason, they break the rules, and contain a byte
    // (value 0) even if the header says there is no data. If
    // Morrowind accepts it, so should we.
    if(c.leftSub == 0)
      {
        // Skip the following zero byte
        c.leftRec--;
        char c;
        esm->read(&c,1);
        return "";
      }

    return getString(c.leftSub);
  }

  // Read the given number of bytes from a subrecord
  void getHExact(void*p, int size)
  {
    getSubHeader();
    if(size !=static_cast<int> (c.leftSub))
      fail("getHExact() size mismatch");
    getExact(p,size);
  }

  // Read the given number of bytes from a named subrecord
  void getHNExact(void*p, int size, const char* name)
  {
    getSubNameIs(name);
    getHExact(p,size);
  }

  /*************************************************************************
   *
   *  Low level sub-record methods
   *
   *************************************************************************/

  // Get the next subrecord name and check if it matches the parameter
  void getSubNameIs(const char* name)
  {
    getSubName();
    if(c.subName != name)
      fail("Expected subrecord " + std::string(name) + " but got " + c.subName.toString());
  }

  /** Checks if the next sub record name matches the parameter. If it
      does, it is read into 'subName' just as if getSubName() was
      called. If not, the read name will still be available for future
      calls to getSubName(), isNextSub() and getSubNameIs().
   */
  bool isNextSub(const char* name)
  {
    if(!c.leftRec) return false;

    getSubName();

    // If the name didn't match, then mark the it as 'cached' so it's
    // available for the next call to getSubName.
    c.subCached = (c.subName != name);

    // If subCached is false, then subName == name.
    return !c.subCached;
  }

  // Read subrecord name. This gets called a LOT, so I've optimized it
  // slightly.
  void getSubName()
    {
      // If the name has already been read, do nothing
      if(c.subCached)
        {
          c.subCached = false;
          return;
        }

      // Don't bother with error checking, we will catch an EOF upon
      // reading the subrecord data anyway.
      esm->read(c.subName.name, 4);
      c.leftRec -= 4;
    }

  // This is specially optimized for LoadINFO.
  bool isEmptyOrGetName()
    {
      if(c.leftRec)
	{
	  esm->read(c.subName.name, 4);
	  c.leftRec -= 4;
	  return false;
	}
      return true;
    }

  // Skip current sub record, including header (but not including
  // name.)
  void skipHSub()
    {
      getSubHeader();
      skip(c.leftSub);
    }

  // Skip sub record and check its size
  void skipHSubSize(int size)
    {
      skipHSub();
      if(static_cast<int> (c.leftSub) != size)
	fail("skipHSubSize() mismatch");
    }

  /* Sub-record header. This updates leftRec beyond the current
     sub-record as well. leftSub contains size of current sub-record.
  */
  void getSubHeader()
    {
      if(c.leftRec < 4)
	fail("End of record while reading sub-record header");

      // Get subrecord size
      getT(c.leftSub);

      // Adjust number of record bytes left
      c.leftRec -= c.leftSub + 4;

      // Check that sizes added up
      if(c.leftRec < 0)
	fail("Not enough bytes left in record for this subrecord.");
    }

  /** Get sub header and check the size
   */
  void getSubHeaderIs(int size)
  {
    getSubHeader();
    if(size != static_cast<int> (c.leftSub))
      fail("getSubHeaderIs(): Sub header mismatch");
  }

  /*************************************************************************
   *
   *  Low level record methods
   *
   *************************************************************************/

  // Get the next record name
  NAME getRecName()
  {
    if(!hasMoreRecs())
      fail("No more records, getRecName() failed");
    getName(c.recName);
    c.leftFile -= 4;

    // Make sure we don't carry over any old cached subrecord
    // names. This can happen in some cases when we skip parts of a
    // record.
    c.subCached = false;

    return c.recName;
  }

  // Skip the rest of this record. Assumes the name and header have
  // already been read
  void skipRecord()
    {
      skip(c.leftRec);
      c.leftRec = 0;
    }

  // Skip an entire record, including the header (but not the name)
  void skipHRecord()
  {
    if(!c.leftFile) return;
    getRecHeader();
    skipRecord();
  }

  /* Read record header. This updatesleftFile BEYOND the data that
     follows the header, ie beyond the entire record. You should use
     leftRec to orient yourself inside the record itself.
  */
  void getRecHeader() { uint32_t u; getRecHeader(u); }
  void getRecHeader(uint32_t &flags)
    {
      // General error checking
      if(c.leftFile < 12)
	fail("End of file while reading record header");
      if(c.leftRec)
	fail("Previous record contains unread bytes");

      getUint(c.leftRec);
      getUint(flags);// This header entry is always zero
      getUint(flags);
      c.leftFile -= 12;

      // Check that sizes add up
      if(c.leftFile < c.leftRec)
	fail("Record size is larger than rest of file");

      // Adjust number of bytes c.left in file
      c.leftFile -= c.leftRec;
    }

  bool hasMoreRecs() { return c.leftFile > 0; }
  bool hasMoreSubs() { return c.leftRec > 0; }


  /*************************************************************************
   *
   *  Lowest level data reading and misc methods
   *
   *************************************************************************/

  template <typename X>
  void getT(X &x) { getExact(&x, sizeof(X)); }

  void getExact(void*x, int size)
  {
    int t = esm->read(x, size);
    if(t != size)
      fail("Read error");
  }
  void getName(NAME &name) { getT(name); }
  void getUint(uint32_t &u) { getT(u); }

  // Read the next size bytes and return them as a string
  std::string getString(int size)
  {
    // Not very optimized, but we can fix that later
    char *ptr = new char[size];
    esm->read(ptr,size);

    // Remove any zero terminators
    for(int i=0; i<size; i++)
      if(ptr[i] == 0)
        size = i;

    // Convert to std::string and return
    std::string res(ptr,size);
    delete[] ptr;
    return res;
  }

  void skip(int bytes) { esm->seek(esm->tell()+bytes); }
  uint64_t getOffset() { return esm->tell(); }

  /// Used for error handling
  void fail(const std::string &msg)
    {
      using namespace std;

      stringstream ss;

      ss << "ESM Error: " << msg;
      ss << "\n  File: " << c.filename;
      ss << "\n  Record: " << c.recName.toString();
      ss << "\n  Subrecord: " << c.subName.toString();
      if(esm != NULL)
        ss << "\n  Offset: 0x" << hex << esm->tell();
      throw str_exception(ss.str());
    }

private:
  Mangle::Stream::StreamPtr esm;

  ESM_Context c;

  // Special file signifier (see SpecialFile enum above)
  int spf;

  SaveData saveData;
  MasterList masters;
};
}
#endif
