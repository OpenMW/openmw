#ifndef OPENMW_ESM_CELL_H
#define OPENMW_ESM_CELL_H

#include <string>

#include "esmcommon.hpp"
#include "defs.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

/* Cell reference. This represents ONE object (of many) inside the
   cell. The cell references are not loaded as part of the normal
   loading process, but are rather loaded later on demand when we are
   setting up a specific cell.
 */
class CellRef
{
public:
  int mRefnum;           // Reference number
  std::string mRefID;    // ID of object being referenced

  float mScale;          // Scale applied to mesh

  // The NPC that owns this object (and will get angry if you steal
  // it)
  std::string mOwner;

  // I have no idea, looks like a link to a global variable?
  std::string mGlob;

  // ID of creature trapped in this soul gem (?)
  std::string mSoul;

  // ?? CNAM has a faction name, might be for objects/beds etc
  // belonging to a faction.
  std::string mFaction;

  // INDX might be PC faction rank required to use the item? Sometimes
  // is -1, which I assume means "any rank".
  int mFactIndex;

  // Depends on context - possibly weapon health, number of uses left
  // or weapon magic charge?
  float mCharge;

  // I have no idea, these are present some times, often along with
  // owner (ANAM) and sometimes otherwise. They are often (but not
  // always) 1. INTV is big for lights (possibly a float?), might have
  // something to do with remaining light "charge".
  int mIntv, mNam9;

  // For doors - true if this door teleports to somewhere else, false
  // if it should open through animation.
  bool mTeleport;

  // Teleport location for the door, if this is a teleporting door.
  Position mDoorDest;

  // Destination cell for doors (optional)
  std::string mDestCell;

  // Lock level for doors and containers
  int mLockLevel;
  std::string mKey, mTrap; // Key and trap ID names, if any

  // No idea - occurs ONCE in Morrowind.esm, for an activator
  char mUnam;

  // Occurs in Tribunal.esm, eg. in the cell "Mournhold, Plaza
  // Brindisi Dorom", where it has the value 100. Also only for
  // activators.
  int mFltv;
  int mNam0;

  // Position and rotation of this object within the cell
  Position mPos;

  void save(ESMWriter &esm);
};

/* Cells hold data about objects, creatures, statics (rocks, walls,
   buildings) and landscape (for exterior cells). Cells frequently
   also has other associated LAND and PGRD records. Combined, all this
   data can be huge, and we cannot load it all at startup. Instead,
   the strategy we use is to remember the file position of each cell
   (using ESMReader::getContext()) and jumping back into place
   whenever we need to load a given cell.
 */
struct Cell
{
  enum Flags
    {
      Interior  = 0x01, // Interior cell
      HasWater  = 0x02, // Does this cell have a water surface
      NoSleep   = 0x04, // Is it allowed to sleep here (without a bed)
      QuasiEx   = 0x80  // Behave like exterior (Tribunal+), with
                        // skybox and weather
    };

  struct DATAstruct
  {
    int mFlags;
    int mX, mY;
  };

  struct AMBIstruct
  {
    Color mAmbient, mSunlight, mFog;
    float mFogDensity;
  };

  // Interior cells are indexed by this (it's the 'id'), for exterior
  // cells it is optional.
  std::string mName;

  // Optional region name for exterior and quasi-exterior cells.
  std::string mRegion;

  ESM_Context mContext; // File position
  DATAstruct mData;
  AMBIstruct mAmbi;
  float mWater; // Water level
  bool mWaterInt;
  int mMapColor;
  int mNAM0;

  void load(ESMReader &esm);
  void save(ESMWriter &esm);

  bool isExterior() const
  {
      return !(mData.mFlags & Interior);
  }

  int getGridX() const
  {
      return mData.mX;
  }

  int getGridY() const
  {
      return mData.mY;
  }

  // Restore the given reader to the stored position. Will try to open
  // the file matching the stored file name. If you want to read from
  // somewhere other than the file system, you need to pre-open the
  // ESMReader, and the filename must match the stored filename
  // exactly.
  void restore(ESMReader &esm) const;

  std::string getDescription() const;
  ///< Return a short string describing the cell (mostly used for debugging/logging purpose)

  /* Get the next reference in this cell, if any. Returns false when
     there are no more references in the cell.

     All fields of the CellRef struct are overwritten. You can safely
     reuse one memory location without blanking it between calls.
  */
  static bool getNextRef(ESMReader &esm, CellRef &ref);
};
}
#endif
