#ifndef OPENMW_ESM_NPC_H
#define OPENMW_ESM_NPC_H

#include <string>
#include <vector>

#include "defs.hpp"
#include "loadcont.hpp"
#include "aipackage.hpp"
#include "spelllist.hpp"
#include "loadskil.hpp"

namespace ESM {

class ESMReader;
class ESMWriter;

/*
 * NPC definition
 */

struct NPC
{
    static unsigned int sRecordId;

  // Services
  enum Services
    {
      // This merchant buys:
      Weapon        = 0x00001,
      Armor         = 0x00002,
      Clothing      = 0x00004,
      Books         = 0x00008,
      Ingredients   = 0x00010,
      Picks         = 0x00020,
      Probes        = 0x00040,
      Lights        = 0x00080,
      Apparatus     = 0x00100,
      RepairItem    = 0x00200,
      Misc          = 0x00400,
      Potions       = 0x02000,

      // Other services
      Spells        = 0x00800,
      MagicItems    = 0x01000,
      Training      = 0x04000, // What skills?
      Spellmaking   = 0x08000,
      Enchanting    = 0x10000,
      Repair        = 0x20000
    };

  enum Flags
    {
      Female    = 0x0001,
      Essential = 0x0002,
      Respawn   = 0x0004,
      Autocalc  = 0x0008,
      Skeleton  = 0x0400, // Skeleton blood effect (white)
      Metal     = 0x0800  // Metal blood effect (golden?)
    };

  enum NpcType
  {
    NPC_WITH_AUTOCALCULATED_STATS = 12,
    NPC_DEFAULT = 52
  };

    #pragma pack(push)
    #pragma pack(1)

    struct NPDTstruct52
    {
        short mLevel;
        unsigned char mStrength,
             mIntelligence,
             mWillpower,
             mAgility,
             mSpeed,
             mEndurance,
             mPersonality,
             mLuck;

        char mSkills[Skill::Length];
        char mReputation;
        unsigned short mHealth, mMana, mFatigue;
        char mDisposition, mFactionID, mRank;
        char mUnknown;
        int mGold;
    }; // 52 bytes

    struct NPDTstruct12
    {
        short mLevel;
        char mDisposition, mReputation, mRank;
        char mUnknown1, mUnknown2, mUnknown3;
        int mGold; // ?? not certain
    }; // 12 bytes

    struct Dest
    {
        Position    mPos;
        std::string mCellName;
    };
    #pragma pack(pop)

    char mNpdtType;
    NPDTstruct52 mNpdt52;
    NPDTstruct12 mNpdt12; //for autocalculated characters

    int mFlags;

    bool mPersistent;

    InventoryList mInventory;
    SpellList mSpells;

    AIData mAiData;
    bool mHasAI;

    std::vector<Dest> mTransport;
    AIPackageList     mAiPackage;

    std::string mId, mName, mModel, mRace, mClass, mFaction, mScript;

    // body parts
    std::string mHair, mHead;

    void load(ESMReader &esm);
    void save(ESMWriter &esm) const;

    bool isMale() const;

    void setIsMale(bool value);

    void blank();
    ///< Set record to default state (does not touch the ID).
};
}
#endif
