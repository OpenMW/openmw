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
        Variant mValue;

#ifdef OPENMW_ESM_ENABLE_CPP11_MOVE
        OPENMW_ESM_DEFINE_CPP11_MOVE_OPS(SelectStruct)
        
        static void copy (SelectStruct & d, SelectStruct const & s)
        {
            d.mSelectRule   = s.mSelectRule;
            d.mValue        = s.mValue;
        }

        static void move (SelectStruct & d, SelectStruct & s)
        {
            d.mSelectRule   = std::move (s.mSelectRule);
            d.mValue        = std::move (s.mValue);
        }
#endif // OPENMW_ESM_ENABLE_CPP11_MOVE
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

        REC_BNAM = 0x4d414e42,
        REC_QSTN = 0x4e545351,
        REC_QSTF = 0x46545351,
        REC_QSTR = 0x52545351,
        REC_DELE = 0x454c4544
    };

    void load(ESMReader &esm);
    void save(ESMWriter &esm);

#ifdef OPENMW_ESM_ENABLE_CPP11_MOVE
    OPENMW_ESM_DEFINE_CPP11_MOVE_OPS(DialInfo)

    static void copy (DialInfo & d, DialInfo const & s)
    {
        d.mData             = s.mData;
        d.mSelects          = s.mSelects;
        d.mId               = s.mId;
        d.mPrev             = s.mPrev;
        d.mNext             = s.mNext;
        d.mActor            = s.mActor;
        d.mRace             = s.mRace;
        d.mClass            = s.mClass;
        d.mNpcFaction       = s.mNpcFaction;
        d.mPcFaction        = s.mPcFaction;
        d.mCell             = s.mCell;
        d.mSound            = s.mSound;
        d.mResponse         = s.mResponse;
        d.mResultScript     = s.mResultScript;
        d.mFactionLess      = s.mFactionLess;
        d.mQuestStatus      = s.mQuestStatus;
    }

    static void move (DialInfo & d, DialInfo & s)
    {
        d.mData         = std::move (s.mData);
        d.mSelects      = std::move (s.mSelects);
        d.mId           = std::move (s.mId);
        d.mPrev         = std::move (s.mPrev);
        d.mNext         = std::move (s.mNext);
        d.mActor        = std::move (s.mActor);
        d.mRace         = std::move (s.mRace);
        d.mClass        = std::move (s.mClass);
        d.mNpcFaction   = std::move (s.mNpcFaction);
        d.mPcFaction    = std::move (s.mPcFaction);
        d.mCell         = std::move (s.mCell);
        d.mSound        = std::move (s.mSound);
        d.mResponse     = std::move (s.mResponse);
        d.mResultScript = std::move (s.mResultScript);
        d.mFactionLess  = std::move (s.mFactionLess);
        d.mQuestStatus  = std::move (s.mQuestStatus);
    }
#endif // OPENMW_ESM_ENABLE_CPP11_MOVE
};

}
#endif
