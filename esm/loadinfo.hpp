#ifndef _ESM_INFO_H
#define _ESM_INFO_H

#include "esm_reader.hpp"
#include "defs.hpp"

namespace ESM {

// NOT DONE

/*
 * Dialogue information. A series of these follow after DIAL records,
 * and form a linked list of dialogue items.
 */

struct DialInfo
{
  enum Gender
    {
      Male = 0,
      Female = 1,
      NA = -1
    };

  struct DATAstruct
  {
    int unknown1;
    int disposition;
    char rank;     // Rank of NPC
    char gender;   // See Gender enum
    char PCrank;   // Player rank
    char unknown2;
  }; // 12 bytes
  DATAstruct data;

  // The rules for whether or not we will select this dialog item.
  struct SelectStruct
  {
    std::string selectRule;     // This has a complicated format
    float f;                    // Only one of 'f' or 'i' is used
    int i;
    VarType type;
  };

  // Journal quest indices (introduced with the quest system in Tribunal)
  enum QuestStatus
    {
      QS_None,
      QS_Name,
      QS_Finished,
      QS_Restart,
      QS_Deleted
    };

  // Rules for when to include this item in the final list of options
  // visible to the player.
  std::vector<SelectStruct> selects;

  // Id of this, previous and next INFO items
  std::string id, prev, next,

  // Various references used in determining when to select this item.
    actor, race, clas, npcFaction, pcFaction, cell,

  // Sound and text associated with this item
    sound, response,

  // Result script (uncomiled) to run whenever this dialog item is
  // selected
    resultScript;

  // ONLY include this item the NPC is not part of any faction.
  bool factionLess;

  // Status of this quest item
  QuestStatus questStatus;

  // Hexadecimal versions of the various subrecord names.
  enum SubNames
    {
      REC_ONAM = 0x4d414e4f,
      REC_RNAM = 0x4d414e52,
      REC_CNAM = 0x4d414e43,
      REC_FNAM = 0x4d414e46,
      REC_ANAM = 0x4d414e41,
      REC_DNAM = 0x4d414e44,
      REC_SNAM = 0x4d414e53,
      REC_NAME = 0x454d414e,
      REC_SCVR = 0x52564353,
      REC_INTV = 0x56544e49,
      REC_FLTV = 0x56544c46,
      REC_BNAM = 0x4d414e42,
      REC_QSTN = 0x4e545351,
      REC_QSTF = 0x46545351,
      REC_QSTR = 0x52545351,
      REC_DELE = 0x454c4544
    };

  void load(ESMReader &esm)
  {
    id = esm.getHNString("INAM");
    prev = esm.getHNString("PNAM");
    next = esm.getHNString("NNAM");

    // Not present if deleted
    if(esm.isNextSub("DATA"))
      esm.getHT(data, 12);

    // What follows is somewhat spaghetti-ish, but it's worth if for
    // an extra speedup. INFO is by far the most common record type.

    // subName is a reference to the original, so it changes whenever
    // a new sub name is read. esm.isEmptyOrGetName() will get the
    // next name for us, or return true if there are no more records.
    esm.getSubName();
    const NAME &subName = esm.retSubName();

    if(subName.val == REC_ONAM)
      {
        actor = esm.getHString();
	if(esm.isEmptyOrGetName()) return;
      }
    if(subName.val == REC_RNAM)
      {
        race = esm.getHString();
	if(esm.isEmptyOrGetName()) return;
      }
    if(subName.val == REC_CNAM)
      {
        clas = esm.getHString();
	if(esm.isEmptyOrGetName()) return;
      }

    factionLess = false;
    if(subName.val == REC_FNAM)
      {
        npcFaction = esm.getHString();
	if(npcFaction == "FFFF") factionLess = true;
	if(esm.isEmptyOrGetName()) return;
      }
    if(subName.val == REC_ANAM)
      {
	cell = esm.getHString();
	if(esm.isEmptyOrGetName()) return;
      }
    if(subName.val == REC_DNAM)
      {
        pcFaction = esm.getHString();
	if(esm.isEmptyOrGetName()) return;
      }
    if(subName.val == REC_SNAM)
      {
        sound = esm.getHString();
	if(esm.isEmptyOrGetName()) return;
      }
    if(subName.val == REC_NAME)
      {
	response = esm.getHString();
	if(esm.isEmptyOrGetName()) return;
      }

    while(subName.val == REC_SCVR)
      {
        SelectStruct ss;

        ss.selectRule = esm.getHString();
        esm.isEmptyOrGetName();

        if(subName.val == REC_INTV)
          {
            ss.type = VT_Int;
            esm.getHT(ss.i);
          }
        else if(subName.val == REC_FLTV)
          {
            ss.type = VT_Float;
            esm.getHT(ss.f);
          }
        else 
          esm.fail("INFO.SCVR must precede INTV or FLTV, not "
		   + subName.toString());

        selects.push_back(ss);

	if(esm.isEmptyOrGetName()) return;
      }

    if(subName.val == REC_BNAM)
      {
	resultScript = esm.getHString();
	if(esm.isEmptyOrGetName()) return;
      }

    questStatus = QS_None;
    int skip = 1;

    if     (subName.val == REC_QSTN) questStatus = QS_Name;
    else if(subName.val == REC_QSTF) questStatus = QS_Finished;
    else if(subName.val == REC_QSTR) questStatus = QS_Restart;
    else if(subName.val == REC_DELE) {questStatus = QS_Deleted; skip = 4;}
    else
      esm.fail("Don't know what to do with " + subName.toString() + " in INFO " + id);

    if(questStatus != QS_None)
      esm.skip(skip);
  }
};

/*
  Some old and unused D code and comments, that might be useful later:
  --------

  // We only need to put each item in ONE list. For if your NPC
  // matches this response, then it must match ALL criteria, thus it
  // will have to look up itself in all the lists. I think the order
  // is well optimized in making the lists as small as possible.
  if(this.actor.index != -1) actorDial[this.actor][parent]++;
  else if(cell != "") cellDial[cell][parent]++;
  else if(this.Class != -1) classDial[this.Class][parent]++;
  else if(this.npcFaction != -1)
    factionDial[this.npcFaction][parent]++;
  else if(this.race != -1) raceDial[this.race][parent]++;
  else allDial[parent]++; // Lists dialogues that might
  // apply to all npcs.
  */

// List of dialogue topics (and greetings, voices, etc.) that
// reference other objects. Eg. raceDial is indexed by the indices of
// all races referenced. The value of raceDial is a new AA, which is
// basically used as a map (the int value is just a count and isn't
// used for anything important.) The indices (or elements of the map)
// are the dialogues that reference the given race. I use an AA
// instead of a list or array, since each dialogue can be added lots
// of times.

/*
int allDial[Dialogue*];
int classDial[int][Dialogue*];
int factionDial[int][Dialogue*];
int actorDial[Item][Dialogue*];
// If I look up cells on cell load, I don't have to resolve these
// names into anything!
int cellDial[char[]][Dialogue*];
int raceDial[int][Dialogue*];
*/
}
#endif
