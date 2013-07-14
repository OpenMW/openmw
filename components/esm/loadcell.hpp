#ifndef OPENMW_ESM_CELL_H
#define OPENMW_ESM_CELL_H

#include <string>
#include <vector>
#include <list>

#include "esmcommon.hpp"
#include "defs.hpp"
#include "cellref.hpp"

namespace MWWorld
{
    class ESMStore;
}

namespace ESM
{

class ESMReader;
class ESMWriter;

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

  void preLoad(ESMReader &esm);
  void postLoad(ESMReader &esm);

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

    void blank();
    ///< Set record to default state (does not touch the ID/index).
};
}
#endif
