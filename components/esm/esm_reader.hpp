#ifndef _ESM_READER_H
#define _ESM_READER_H

#include <libs/platform/stdint.h>
#include <libs/platform/string.h>
#include <cassert>
#include <vector>
#include <sstream>

#include <OgreDataStream.h>

#include <components/misc/stringops.hpp>

#include <components/to_utf8/to_utf8.hpp>

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

  int getVer() const { return mCtx.header.version; }
  float getFVer() { if(mCtx.header.version == VER_12) return 1.2; else return 1.3; }
  int getSpecial() const { return mSpf; }
  const std::string getAuthor() { return mCtx.header.author.toString(); }
  const std::string getDesc() { return mCtx.header.desc.toString(); }
  const SaveData &getSaveData() const { return mSaveData; }
  const MasterList &getMasters() { return mMasters; }
  const NAME &retSubName() { return mCtx.subName; }
  uint32_t getSubSize() const { return mCtx.leftSub; }

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
  void restoreContext(const ESM_Context &rc);

  /** Close the file, resets all information. After calling close()
      the structure may be reused to load a new file.
  */
  void close();

  /// Raw opening. Opens the file and sets everything up but doesn't
  /// parse the header.
  void openRaw(Ogre::DataStreamPtr _esm, const std::string &name);

  /// Load ES file from a new stream, parses the header. Closes the
  /// currently open file first, if any.
  void open(Ogre::DataStreamPtr _esm, const std::string &name);

  void open(const std::string &file);

  void openRaw(const std::string &file);

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

  int64_t getHNLong(const char *name);

  // Get data of a given type/size, including subrecord header
  template <typename X>
  void getHT(X &x)
  {
      getSubHeader();
      if (mCtx.leftSub != sizeof(X))
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
  std::string getHNOString(const char* name);

  // Read a string with the given sub-record name
  std::string getHNString(const char* name);

  // Read a string, including the sub-record header (but not the name)
  std::string getHString();

  // Read the given number of bytes from a subrecord
  void getHExact(void*p, int size);

  // Read the given number of bytes from a named subrecord
  void getHNExact(void*p, int size, const char* name);

  /*************************************************************************
   *
   *  Low level sub-record methods
   *
   *************************************************************************/

  // Get the next subrecord name and check if it matches the parameter
  void getSubNameIs(const char* name);

  /** Checks if the next sub record name matches the parameter. If it
      does, it is read into 'subName' just as if getSubName() was
      called. If not, the read name will still be available for future
      calls to getSubName(), isNextSub() and getSubNameIs().
   */
  bool isNextSub(const char* name);

  // Read subrecord name. This gets called a LOT, so I've optimized it
  // slightly.
  void getSubName();

  // This is specially optimized for LoadINFO.
  bool isEmptyOrGetName();

  // Skip current sub record, including header (but not including
  // name.)
  void skipHSub();

  // Skip sub record and check its size
  void skipHSubSize(int size);

  /* Sub-record header. This updates leftRec beyond the current
     sub-record as well. leftSub contains size of current sub-record.
  */
  void getSubHeader();

  /** Get sub header and check the size
   */
  void getSubHeaderIs(int size);

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

  // Skip an entire record, including the header (but not the name)
  void skipHRecord();

  /* Read record header. This updatesleftFile BEYOND the data that
     follows the header, ie beyond the entire record. You should use
     leftRec to orient yourself inside the record itself.
  */
  void getRecHeader() { uint32_t u; getRecHeader(u); }
  void getRecHeader(uint32_t &flags);

  bool hasMoreRecs() const { return mCtx.leftFile > 0; }
  bool hasMoreSubs() const { return mCtx.leftRec > 0; }


  /*************************************************************************
   *
   *  Lowest level data reading and misc methods
   *
   *************************************************************************/

  template <typename X>
  void getT(X &x) { getExact(&x, sizeof(X)); }

  void getExact(void*x, int size);
  void getName(NAME &name) { getT(name); }
  void getUint(uint32_t &u) { getT(u); }

  // Read the next 'size' bytes and return them as a string. Converts
  // them from native encoding to UTF8 in the process.
  std::string getString(int size);

  void skip(int bytes) { mEsm->seek(mEsm->tell()+bytes); }
  uint64_t getOffset() { return mEsm->tell(); }

  /// Used for error handling
  void fail(const std::string &msg);

  /// Sets font encoding for ESM strings
  void setEncoding(const std::string& encoding);

private:
  Ogre::DataStreamPtr mEsm;

  ESM_Context mCtx;

  // Special file signifier (see SpecialFile enum above)
  int mSpf;

  SaveData mSaveData;
  MasterList mMasters;
  ToUTF8::FromType mEncoding;
};
}
#endif
