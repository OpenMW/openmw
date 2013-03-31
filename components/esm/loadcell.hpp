#ifndef OPENMW_ESM_CELL_H
#define OPENMW_ESM_CELL_H

#include <string>
#include <vector>
#include <list>

#include "esmcommon.hpp"
#include "defs.hpp"


namespace MWWorld
{
    class ESMStore;
}


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

  // For weapon or armor, this is the remaining item health.
  // For tools (lockpicks, probes, repair hammer) it is the remaining uses.
  int mCharge;

  int mNam9;

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
  signed char mUnam;

  // Track deleted references. 0 - not deleted, 1 - deleted, but respawns, 2 - deleted and does not respawn.
  int mDeleted;

  // Occurs in Tribunal.esm, eg. in the cell "Mournhold, Plaza
  // Brindisi Dorom", where it has the value 100. Also only for
  // activators.
  int mFltv;
  int mNam0;

  // Position and rotation of this object within the cell
  Position mPos;

  void save(ESMWriter &esm);
};

/* Moved cell reference tracking object. This mainly stores the target cell
        of the reference, so we can easily know where it has been moved when another
        plugin tries to move it independently.
    Unfortunately, we need to implement this here.
    */
class MovedCellRef
{
public:
    int mRefnum;

    // Target cell (if exterior)
    int mTarget[2];

    // TODO: Support moving references between exterior and interior cells!
    //  This may happen in saves, when an NPC follows the player. Tribunal
    //  introduces a henchman (which no one uses), so we may need this as well.
};

/// Overloaded copare operator used to search inside a list of cell refs.
bool operator==(const MovedCellRef& ref, int pRefnum);
bool operator==(const CellRef& ref, int pRefnum);

typedef std::list<MovedCellRef> MovedCellRefTracker;
typedef std::list<CellRef> CellRefTracker;

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

  std::vector<ESM_Context> mContextList; // File position; multiple positions for multiple plugin support
  DATAstruct mData;
  AMBIstruct mAmbi;
  float mWater; // Water level
  bool mWaterInt;
  int mMapColor;
  int mNAM0;

  // References "leased" from another cell (i.e. a different cell
  //  introduced this ref, and it has been moved here by a plugin)
  CellRefTracker mLeasedRefs;
  MovedCellRefTracker mMovedRefs;

  void load(ESMReader &esm, MWWorld::ESMStore &store);

  // This method is left in for compatibility with esmtool. Parsing moved references currently requires
  //  passing ESMStore, bit it does not know about this parameter, so we do it this way.
  void load(ESMReader &esm, bool saveContext = true);
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
  void restore(ESMReader &esm, int iCtx) const;

  std::string getDescription() const;
  ///< Return a short string describing the cell (mostly used for debugging/logging purpose)

  /* Get the next reference in this cell, if any. Returns false when
     there are no more references in the cell.

     All fields of the CellRef struct are overwritten. You can safely
     reuse one memory location without blanking it between calls.
  */
  static bool getNextRef(ESMReader &esm, CellRef &ref);

  /* This fetches an MVRF record, which is used to track moved references.
   * Since they are comparably rare, we use a separate method for this.
   */
  static bool getNextMVRF(ESMReader &esm, MovedCellRef &mref);
};
}
#endif
