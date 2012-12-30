#ifndef OPENMW_ESM_INFO_H
#define OPENMW_ESM_INFO_H

#include <string>
#include <vector>

#include "defs.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

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
        int mUnknown1;
        int mDisposition;
        signed char mRank; // Rank of NPC
        signed char mGender; // See Gender enum
        signed char mPCrank; // Player rank
        signed char mUnknown2;
    }; // 12 bytes
    DATAstruct mData;

    // The rules for whether or not we will select this dialog item.
    struct SelectStruct
    {
        std::string mSelectRule; // This has a complicated format
        float mF; // Only one of 'f' or 'i' is used
        int mI;
        VarType mType;
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
    std::vector<SelectStruct> mSelects;

    // Id of this, previous and next INFO items
    std::string mId, mPrev, mNext;

    // Various references used in determining when to select this item.
    std::string mActor, mRace, mClass, mNpcFaction, mPcFaction, mCell;

    // Sound and text associated with this item
    std::string mSound, mResponse;

    // Result script (uncomiled) to run whenever this dialog item is
    // selected
    std::string mResultScript;

    // ONLY include this item the NPC is not part of any faction.
    bool mFactionLess;

    // Status of this quest item
    QuestStatus mQuestStatus;

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

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
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
