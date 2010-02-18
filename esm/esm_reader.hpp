#ifndef _ESM_READER_H
#define _ESM_READER_H

#include <string>
#include <stdint.h>
#include <string.h>
#include <assert.h>
#include <vector>

#include "../mangle/stream/stream.h"
#include "../mangle/stream/servers/file_stream.h"
#include "../mangle/tools/str_exception.h"
#include "../tools/stringops.h"

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

  std::string toString() { return std::string(name, strnlen(name, LEN)); }
};

typedef NAME_T<4> NAME;
typedef NAME_T<32> NAME32;
typedef NAME_T<64> NAME64;
typedef NAME_T<256> NAME256;

class ESMReader
{
public:

  /*************************************************************************
   *
   *  Public type definitions
   *
   *************************************************************************/

#pragma pack(push)
#pragma pack(1)
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

  typedef std::vector<MasterData> MasterList;

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

  /*************************************************************************
   *
   *  Information retrieval
   *
   *************************************************************************/

  int getVer() { return header.version; }
  float getFVer() { return *((float*)&header.version); }
  int getSpecial() { return spf; }
  const std::string getAuthor() { return header.author.toString(); }
  const std::string getDesc() { return header.desc.toString(); }
  const SaveData &getSaveData() { return saveData; }
  const MasterList &getMasters() { return masters; }


  /*************************************************************************
   *
   *  Opening and closing
   *
   *************************************************************************/

  /// Close the file, resets all information. After calling close()
  /// the structure may be reused to load a new file.
  void close()
  {
    esm.reset();
    filename.clear();
    leftFile = 0;
    leftRec = 0;
    leftSub = 0;
    subCached = false;
    recName.val = 0;
    subName.val = 0;
  }

  /// Load ES file from a new stream. Calls close() automatically.
  void open(Mangle::Stream::StreamPtr _esm, const std::string &name)
  {
    close();
    esm = _esm;
    filename = name;
    leftFile = esm->size();

    // Flag certain files for special treatment, based on the file
    // name.
    const char *cstr = filename.c_str();
    if(iends(cstr, "Morrowind.esm")) spf = SF_Morrowind;
    else if(iends(cstr, "Tribunal.esm")) spf = SF_Tribunal;
    else if(iends(cstr, "Bloodmoon.esm")) spf = SF_Bloodmoon;
    else spf = SF_Other;

    if(getRecName() != "TES3")
      fail("Not a valid Morrowind file");

    getRecHeader();

    // Get the header
    getHNT(header, "HEDR", 300);

    if(header.version != VER_12 &&
       header.version != VER_13)
      fail("Unsupported file format version");

    while(isNextSub("MAST"))
      {
        MasterData m;
        m.name = getHString();
        m.size = getHNLong("DATA");
        masters.push_back(m);
      }

    if(header.type == FT_ESS)
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
    if(leftSub != sizeof(X))
      fail("getHT(): subrecord size mismatch");
    getT(x);
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
    if(leftSub == 0)
      {
        // Skip the following zero byte
        leftRec--;
        char c;
        esm->read(&c,1);
        return "";
      }

    return getString(leftSub);
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
    if(subName != name)
      fail("Expected subrecord " + std::string(name) + " but got " + subName.toString());
  }

  /** Checks if the next sub record name matches the parameter. If it
      does, it is read into 'subName' just as if getSubName() was
      called. If not, the read name will still be available for future
      calls to getSubName(), isNextSub() and getSubNameIs().
   */
  bool isNextSub(const char* name)
  {
    if(!leftRec) return false;

    getSubName();

    // If the name didn't match, then mark the it as 'cached' so it's
    // available for the next call to getSubName.
    subCached = (subName != name);

    // If subCached is false, then subName == name.
    return !subCached;
  }

  // Read subrecord name. This gets called a LOT, so I've optimized it
  // slightly.
  void getSubName()
    {
      // If the name has already been read, do nothing
      if(subCached)
        {
          subCached = false;
          return;
        }

      // Don't bother with error checking, we will catch an EOF upon
      // reading the subrecord data anyway.
      esm->read(subName.name, 4);
      leftRec -= 4;
    }

  // Skip current sub record, including header (but not including
  // name.)
  void skipHSub()
    {
      getSubHeader();
      skip(leftSub);
    }

  // Skip sub record and check its size
  void skipHSubSize(int size)
    {
      skipHSub();
      if(leftSub != size)
	fail("skipHSubSize() mismatch");
    }

  /* Sub-record head This updates leftRec beyond the current
     sub-record as well. leftSub contains size of current sub-record.
  */
  void getSubHeader()
    {
      if(leftRec < 4)
	fail("End of record while reading sub-record header");

      // Get subrecord size
      getT(leftSub);

      // Adjust number of record bytes left
      leftRec -= leftSub + 4;

      // Check that sizes added up
      if(leftRec < 0)
	fail("Not enough bytes left in record for this subrecord.");
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
    getName(recName);
    leftFile -= 4;
    return recName;
  }

  // Skip the rest of this record. Assumes the name and header have
  // already been read
  void skipRecord()
    {
      skip(leftRec);
      leftRec = 0;
    }

  // Skip an entire record, including the header (but not the name)
  void skipHRecord()
  {
    if(!leftFile) return;
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
      if(leftFile < 12)
	fail("End of file while reading record header");
      if(leftRec)
	fail("Previous record contains unread bytes");

      getUint(leftRec);
      getUint(flags);// This header entry is always zero
      getUint(flags);
      leftFile -= 12;

      // Check that sizes add up
      if(leftFile < leftRec)
	fail("Record size is larger than rest of file");

      // Adjust number of bytes left in file
      leftFile -= leftRec;
    }

  bool hasMoreRecs() { return leftFile > 0; }


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
    // Not very optimized, but we'll fix that later
    char *ptr = new char[size];
    esm->read(ptr,size);
    std::string res(ptr,size);
    delete[] ptr;
    return res;
  }

  void skip(int bytes) { esm->seek(esm->tell()+bytes); }

  /// Used for error handling
  void fail(const std::string &msg)
    {
      std::string err = "ESM Error: " + msg;
      err += "\n  File: " + filename;
      err += "\n  Record: " + recName.toString();
      err += "\n  Subrecord: " + subName.toString();
      throw str_exception(err);
    }

private:
  Mangle::Stream::StreamPtr esm;
  size_t leftFile;
  uint32_t leftRec, leftSub;
  std::string filename;

  NAME recName, subName;

  // True if subName has been read but not used.
  bool subCached;

  HEDRstruct header;
  SaveData saveData;
  MasterList masters;
  int spf; // Special file signifier (see SpecialFile below)
};

#endif
