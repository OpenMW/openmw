#ifndef OPENMW_ESM_COMMON_H
#define OPENMW_ESM_COMMON_H

#include <string>
#include <cstring>

#include <stdint.h>
#include <string.h>

namespace ESM
{
enum Version
  {
    VER_12 =  0x3f99999a,
    VER_13 =  0x3fa66666,
    VER_080 = 0x3f4ccccd, // TES4
    VER_100 = 0x3f800000, // TES4
    VER_132 = 0x3fa8f5c3, // FONV Courier's Stash, DeadMoney
    VER_133 = 0x3faa3d71, // FONV HonestHearts
    VER_134 = 0x3fab851f, // FONV, GunRunnersArsenal, LonesomeRoad, OldWorldBlues
    VER_094 = 0x3f70a3d7, // TES5/FO3
    VER_17 =  0x3fd9999a  // TES5
  };

/* A structure used for holding fixed-length strings. In the case of
   LEN=4, it can be more efficient to match the string as a 32 bit
   number, therefore the struct is implemented as a union with an int.
 */
template <int LEN>
union NAME_T
{
    char name[LEN];
    uint32_t intval;

  bool operator==(const char *str) const
  {
    for(int i=0; i<LEN; i++)
      if(name[i] != str[i]) return false;
      else if(name[i] == 0) return true;
    return str[LEN] == 0;
  }
  bool operator!=(const char *str) const { return !((*this)==str); }

  bool operator==(const std::string &str) const
  {
    return (*this) == str.c_str();
  }
  bool operator!=(const std::string &str) const { return !((*this)==str); }

  bool operator==(uint32_t v) const { return v == intval; }
  bool operator!=(uint32_t v) const { return v != intval; }

  std::string toString() const { return std::string(name, strnlen(name, LEN)); }

  void assign (const std::string& value) { std::strncpy (name, value.c_str(), LEN); }
};

typedef NAME_T<4> NAME;
typedef NAME_T<32> NAME32;
typedef NAME_T<64> NAME64;
typedef NAME_T<256> NAME256;

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
  // When working with multiple esX files, we will generate lists of all files that
  //  actually contribute to a specific cell. Therefore, we need to store the index
  //  of the file belonging to this contest. See CellStore::(list/load)refs for details.
  int TESindex;
  int index;

  // True if subName has been read but not used.
  bool subCached;

  // File position. Only used for stored contexts, not regularly
  // updated within the reader itself.
  size_t filePos;
};

}

#endif
