#include <iostream>
#include <string>
#include <stdint.h>
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
};

typedef NAME_T<4> NAME;
typedef NAME_T<32> NAME32;
typedef NAME_T<256> NAME256;

class ESMReader
{
  Mangle::Stream::StreamPtr esm;
  size_t leftFile;
  uint32_t leftRec;
  string filename;

  NAME recName, subName;

public:
  void open(Mangle::Stream::StreamPtr _esm, const string &name)
  {
    esm = _esm;
    filename = name;
    leftFile = esm->size();
    leftRec = 0;
    recName.val = 0;
    subName.val = 0;

    cout << "left: " << leftFile << endl;
    if(getRecName() != "TES3")
      fail("Not a valid Morrowind file");
    cout << "left: " << leftFile << endl;

    // The flags are always zero
    uint32_t flags;
    getRecHeader(flags);
  }

  void open(const string &file)
  {
    using namespace Mangle::Stream;
    open(StreamPtr(new FileStream(file)), file);
  }

  /*************************************************************************
   *
   *  Low level reading methods
   *
   *************************************************************************/

  NAME getRecName()
  {
    if(!hasMoreRecs())
      fail("No more records, getRecName() failed");
    getName(recName);
    leftFile -= 4;
    return recName;
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

  void getName(NAME &name) { esm->read(&name,4); }
  void getUint(uint32_t &u) { esm->read(&u,4); }

  /// Used for error handling
  void fail(const std::string &msg)
    {
      std::string err = "ESM Error: " + msg;
      err += "\n  File: " + filename;
      err += "\n  Record: " + string(recName.name,4);
      err += "\n  Subrecord: " + string(subName.name,4);
      throw str_exception(err);
    }
};

int main(int argc, char**argv)
{
  if(argc != 2)
    return 1;

  ESMReader esm;
  esm.open(argv[1]);

  return 0;
}
