#ifndef OPENMW_ESM_INFO_H
#define OPENMW_ESM_INFO_H

#include <string>
#include <vector>

#include "defs.hpp"
#include "variant.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Dialogue information. A series of these follow after DIAL records,
 * and form a linked list of dialogue items.
 */

struct DialInfo
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "DialInfo"; }

    enum Gender
    {
        Male = 0,
        Female = 1,
        NA = -1
    };

    struct DATAstruct
    {
        int mUnknown1;
        union
        {
            int mDisposition; // Used for dialogue responses
            int mJournalIndex;  // Used for journal entries
        };
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
        Variant mValue;
    };

    // Journal quest indices (introduced with the quest system in Tribunal)
    enum QuestStatus
    {
        QS_None = 0,
        QS_Name = 1,
        QS_Finished = 2,
        QS_Restart = 3
    };

    // Rules for when to include this item in the final list of options
    // visible to the player.
    std::vector<SelectStruct> mSelects;

    // Id of this, previous and next INFO items
    std::string mId, mPrev, mNext;

    // Various references used in determining when to select this item.
    std::string mActor, mRace, mClass, mFaction, mPcFaction, mCell;

    // Sound and text associated with this item
    std::string mSound, mResponse;

    // Result script (uncompiled) to run whenever this dialog item is
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

        REC_BNAM = 0x4d414e42,
        REC_QSTN = 0x4e545351,
        REC_QSTF = 0x46545351,
        REC_QSTR = 0x52545351,
        REC_DELE = 0x454c4544
    };

    void load(ESMReader &esm, bool &isDeleted);
    ///< Loads Info record

    void save(ESMWriter &esm, bool isDeleted = false) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};

}
#endif
