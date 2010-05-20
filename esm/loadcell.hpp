#ifndef _ESM_CELL_H
#define _ESM_CELL_H

#include "esm_reader.hpp"
#include "defs.hpp"

namespace ESM {

/* Cell reference. This represents ONE object (of many) inside the
   cell. The cell references are not loaded as part of the normal
   loading process, but are rather loaded later on demand when we are
   setting up a specific cell.
 */
struct CellRef
{
  int refnum;           // Reference number
  std::string refID;    // ID of object being referenced

  float scale;          // Scale applied to mesh

  // The NPC that owns this object (and will get angry if you steal
  // it)
  std::string owner;

  // I have no idea, looks like a link to a global variable?
  std::string glob;

  // ID of creature trapped in this soul gem (?)
  std::string soul;

  // ?? CNAM has a faction name, might be for objects/beds etc
  // belonging to a faction.
  std::string faction;

  // INDX might be PC faction rank required to use the item? Sometimes
  // is -1, which I assume means "any rank".
  int factIndex;

  // Depends on context - possibly weapon health, number of uses left
  // or weapon magic charge?
  float charge;

  // I have no idea, these are present some times, often along with
  // owner (ANAM) and sometimes otherwise. They are often (but not
  // always) 1. INTV is big for lights (possibly a float?), might have
  // something to do with remaining light "charge".
  int intv, nam9;

  // For doors - true if this door teleports to somewhere else, false
  // if it should open through animation.
  bool teleport;

  // Teleport location for the door, if this is a teleporting door.
  Position doorDest;

  // Destination cell for doors (optional)
  std::string destCell;

  // Lock level for doors and containers
  int lockLevel;
  std::string key, trap; // Key and trap ID names, if any

  // No idea - occurs ONCE in Morrowind.esm, for an activator
  char unam;

  // Occurs in Tribunal.esm, eg. in the cell "Mournhold, Plaza
  // Brindisi Dorom", where it has the value 100. Also only for
  // activators.
  int fltv;

  // Position and rotation of this object within the cell
  Position pos;
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
    int flags;
    int gridX, gridY;
  };

  struct AMBIstruct
  {
    Color ambient, sunlight, fog;
    float fogDensity;
  };

  // Interior cells are indexed by this (it's the 'id'), for exterior
  // cells it is optional.
  std::string name,

  // Optional region name for exterior and quasi-exterior cells.
    region;

  ESM_Context context; // File position
  DATAstruct data;
  AMBIstruct ambi;
  int water; // Water level
  int mapColor;

  void load(ESMReader &esm)
  {
    // All cells have a name record, even nameless exterior cells.
    name = esm.getHNString("NAME");

    // Ignore this for now, it might mean we should delete the entire
    // cell?
    if(esm.isNextSub("DELE")) esm.skipHSub();

    esm.getHNT(data, "DATA", 12);

    // Water level
    water = 0;

    if(data.flags & Interior)
      {
        // Interior cells

        if(esm.isNextSub("INTV") || esm.isNextSub("WHGT"))
          esm.getHT(water);

        // Quasi-exterior cells have a region (which determines the
        // weather), pure interior cells have ambient lighting
        // instead.
        if(data.flags & QuasiEx)
          region = esm.getHNOString("RGNN");
        else
          esm.getHNT(ambi, "AMBI", 16);
      }
    else
      {
        // Exterior cells
        region = esm.getHNOString("RGNN");
        esm.getHNOT(mapColor, "NAM5");
      }

    // Save position of the cell references and move on
    context = esm.getContext();
    esm.skipRecord();
  }

  // Restore the given reader to the stored position. Will try to open
  // the file matching the stored file name. If you want to read from
  // somewhere other than the file system, you need to pre-open the
  // ESMReader, and the filename must match the stored filename
  // exactly.
  void restore(ESMReader &esm) const
  { esm.restoreContext(context); }

  /* Get the next reference in this cell, if any. Returns false when
     there are no more references in the cell.

     All fields of the CellRef struct are overwritten. You can safely
     reuse one memory location without blanking it between calls.
  */
  static bool getNextRef(ESMReader &esm, CellRef &ref)
  {
    if(!esm.hasMoreSubs()) return false;

    // Number of references in the cell? Maximum once in each cell,
    // but not always at the beginning, and not always right. In other
    // words, completely useless.
    {
      int i;
      esm.getHNOT(i, "NAM0");
    }

    esm.getHNT(ref.refnum, "FRMR");
    ref.refID = esm.getHNString("NAME");

    // getHNOT will not change the existing value if the subrecord is
    // missing
    ref.scale = 1.0;
    esm.getHNOT(ref.scale, "XSCL");

    ref.owner = esm.getHNOString("ANAM");
    ref.glob = esm.getHNOString("BNAM");
    ref.soul = esm.getHNOString("XSOL");

    ref.faction = esm.getHNOString("CNAM");
    ref.factIndex = -1;
    esm.getHNOT(ref.factIndex, "INDX");

    ref.charge = -1.0;
    esm.getHNOT(ref.charge, "XCHG");

    ref.intv = 0;
    ref.nam9 = 0;
    esm.getHNOT(ref.intv, "INTV");
    esm.getHNOT(ref.nam9, "NAM9");

    // Present for doors that teleport you to another cell.
    if(esm.isNextSub("DODT"))
      {
        ref.teleport = true;
        esm.getHT(ref.doorDest);
        ref.destCell = esm.getHNOString("DNAM");
      }
    else ref.teleport = false;

    // Integer, despite the name suggesting otherwise
    esm.getHNOT(ref.lockLevel, "FLTV");
    ref.key = esm.getHNOString("KNAM");
    ref.trap = esm.getHNOString("TNAM");

    ref.unam = 0;
    ref.fltv = 0;
    esm.getHNOT(ref.unam, "UNAM");
    esm.getHNOT(ref.fltv, "FLTV");

    esm.getHNT(ref.pos, "DATA", 24);

    return true;
  }
};
}
#endif
