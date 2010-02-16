#include <iostream>
#include <string>
#include <stdint.h>
#include <string.h>
#include <assert.h>
using namespace std;

#include "../mangle/stream/stream.h"
#include "../mangle/stream/servers/file_stream.h"
#include "../mangle/tools/str_exception.h"

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

  bool operator==(const string &str)
  {
    return (*this) == str.c_str();
  }
  bool operator!=(const string &str) { return !((*this)==str); }

  bool operator==(int v) { return v == val; }
  bool operator!=(int v) { return v != val; }

  string toString() { return string(name, strnlen(name, LEN)); }
};

typedef NAME_T<4> NAME;
typedef NAME_T<32> NAME32;
typedef NAME_T<256> NAME256;

class ESMReader
{
  Mangle::Stream::StreamPtr esm;
  size_t leftFile;
  uint32_t leftRec, leftSub;
  string filename;

  NAME recName, subName;

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
#pragma pack(pop)

  HEDRstruct header;

public:
  enum Version
    {
      VER_12 = 0x3f99999a,
      VER_13 = 0x3fa66666
    };

  enum FileType
    {
      FT_ESP = 0,
      FT_ESM = 1,
      FT_ESS = 32
    };

  void open(Mangle::Stream::StreamPtr _esm, const string &name)
  {
    esm = _esm;
    filename = name;
    leftFile = esm->size();
    leftRec = 0;
    leftSub = 0;
    recName.val = 0;
    subName.val = 0;

    // TODO: determine special file status from file name

    if(getRecName() != "TES3")
      fail("Not a valid Morrowind file");

    // The flags are always zero
    uint32_t flags;
    getRecHeader(flags);

    // Get the header
    getHNT(header, "HEDR");

    if(header.version != VER_12 &&
       header.version != VER_13)
      fail("Unsupported file format version");

    cout << "Author: " << header.author.toString() << endl;
    cout << "Description: " << header.desc.toString() << endl;

    while(isNextSub("MAST"))
      {
        // TODO: read master data here
        skipHSub();
      }

    // TODO: Read extra savegame data
  }

  void open(const string &file)
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

  // Get data of a given type/size, including subrecord header
  template <typename X>
  void getHT(X &x)
  {
    getSubHeader();
    if(leftSub != sizeof(X))
      fail("getHT(): subrecord size mismatch");
    getT(x);
  }

  /*************************************************************************
   *
   *  Low level reading methods
   *
   *************************************************************************/

  // Get the next subrecord name and check if it matches the parameter
  void getSubNameIs(const char* name)
  {
    getSubName();
    if(subName != name)
      fail("Expected subrecord " + string(name) + " but got " + subName.toString());
  }

  // Get the next record name
  NAME getRecName()
  {
    if(!hasMoreRecs())
      fail("No more records, getRecName() failed");
    getName(recName);
    leftFile -= 4;
    return recName;
  }

  // Read subrecord name. I've optimized this slightly, since it gets
  // called a LOT.
  void getSubName()
    {
      // Don't bother with error checking, we will catch an EOF upon
      // reading the subrecord data anyway.
      esm->read(subName.name, 4);
      leftRec -= 4;
    }

  /* Sub-record head This updates leftRec beyond the current
     sub-record as well. leftSub contains size of current sub-record.
  */
  void getSubHeader()
    {
      if(leftRec < 4)
	fail("End of record while reading sub-record header");

      getT(leftSub);

      // Adjust number of record bytes left
      leftRec -= leftSub + 4;

      // Check that sizes add up
      if(leftRec < 0)
	fail("Not enough bytes left in record for this subrecord.");
    }

  /* Read record header. This updatesleftFile BEYOND the data that
     follows the header, ie beyond the entire record. You should use
     leftRec to orient yourself inside the record itself.
  */
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

  template <typename X>
  void getT(X &x) { esm->read(&x, sizeof(X)); }
  void getName(NAME &name) { getT(name); }
  void getUint(uint32_t &u) { getT(u); }

  /// Used for error handling
  void fail(const std::string &msg)
    {
      std::string err = "ESM Error: " + msg;
      err += "\n  File: " + filename;
      err += "\n  Record: " + recName.toString();
      err += "\n  Subrecord: " + subName.toString();
      throw str_exception(err);
    }
};

int main(int argc, char**argv)
{
  if(argc != 2)
    {
      cout << "Specify an ES file\n";
      return 1;
    }

  ESMReader esm;
  esm.open(argv[1]);

  return 0;
}
