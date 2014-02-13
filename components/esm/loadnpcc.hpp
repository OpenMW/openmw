#ifndef OPENMW_ESM_NPCC_H
#define OPENMW_ESM_NPCC_H

#include <string>

// TODO: create implementation files to remove this
#include "esmreader.hpp"

namespace ESM {

class ESMReader;
class ESMWriter;

/*
 * NPC change information (found in savegame files only). We can't
 * read these yet.
 *
 * Some general observations about savegames:
 *
 * Magical items/potions/spells/etc are added normally as new ALCH,
 * SPEL, etc. records, with unique numeric identifiers.
 *
 * Books with ability enhancements are listed in the save if they have
 * been read.
 *
 * GLOB records set global variables.
 *
 * SCPT records do not define new scripts, but assign values to the
 * variables of existing ones.
 *
 * STLN - stolen items, ONAM is the owner
 *
 * GAME - contains a GMDT (game data) of unknown format
 *
 * VFXM, SPLM, KLST - no clue
 *
 * PCDT - seems to contain a lot of DNAMs, strings?
 *
 * FMAP - MAPH and MAPD, probably map data.
 *
 * JOUR - the entire journal in html
 *
 * QUES - seems to contain all the quests in the game, not just the
 * ones you have done or begun.
 *
 * REGN - lists all regions in the game, even unvisited ones.
 *
 * The DIAL/INFO blocks contain changes to characters' dialog status.
 *
 * Dammit there's a lot of stuff in there! Should really have
 * suspected as much. The strategy further is to completely ignore
 * save files for the time being.
 *
 * Several records have a "change" variant, like NPCC, CNTC
 * (contents), and CREC (creature.) These seem to alter specific
 * instances of creatures, npcs, etc. I have not identified most of
 * their subrecords yet.
 *
 * Several NPCC records have names that begin with "chargen ", I don't
 * know if it means something special yet.
 *
 * The CNTC blocks seem to be instances of leveled lists. When a
 * container is supposed to contain this leveled list of this type,
 * but is referenced elsewhere in the file by an INDX, the CNTC with
 * the corresponding leveled list identifier and INDX will determine
 * the container contents instead.
 *
 * Some classes of objects seem to be altered, and these include an
 * INDX, which is probably an index used by specific references other
 * places within the save file. I guess this means 'use this class for
 * these objects, not the general class.' All the indices I have
 * encountered so far are zero, but they have been for different
 * classes (different containers, really) so possibly we start from
 * zero for each class. This looks like a mess, but is probably still
 * easier than to duplicate everything. I think WRITING this format
 * will be harder than reading it.
 */

struct LoadNPCC
{
    static unsigned int sRecordId;

    std::string mId;

  void load(ESMReader &esm)
  {
    esm.skipRecord();
  }
  void save(ESMWriter &esm) const
  {
  }
};
}
#endif
