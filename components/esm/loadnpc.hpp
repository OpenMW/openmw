#ifndef OPENMW_ESM_NPC_H
#define OPENMW_ESM_NPC_H

#include <string>
#include <vector>

#include "defs.hpp"
#include "loadcont.hpp"
#include "aipackage.hpp"
#include "spelllist.hpp"
#include "loadskil.hpp"
#include "transport.hpp"

namespace ESM {

class ESMReader;
class ESMWriter;

/*
 * NPC definition
 */

struct NPC
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "NPC"; }

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

      AllItems = Weapon|Armor|Clothing|Books|Ingredients|Picks|Probes|Lights|Apparatus|RepairItem|Misc|Potions,

      // Other services
      Spells        = 0x00800,
      MagicItems    = 0x01000,
      Training      = 0x04000,
      Spellmaking   = 0x08000,
      Enchanting    = 0x10000,
      Repair        = 0x20000
    };

  enum Flags
    {
      Female        = 0x01,
      Essential     = 0x02,
      Respawn       = 0x04,
      Base          = 0x08,
      Autocalc      = 0x10
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

        // mSkill can grow up to 200, it must be unsigned
        unsigned char mSkills[Skill::Length];

        char mUnknown1;
        unsigned short mHealth, mMana, mFatigue;
        unsigned char mDisposition, mReputation, mRank;
        char mUnknown2;
        int mGold;
    }; // 52 bytes

    //Structure for autocalculated characters.
    // This is only used for load and save operations.
    struct NPDTstruct12
    {
        short mLevel;
        // see above
        unsigned char mDisposition, mReputation, mRank;
        char mUnknown1, mUnknown2, mUnknown3;
        int mGold;
    }; // 12 bytes
    #pragma pack(pop)

    unsigned char mNpdtType;
    //Worth noting when saving the struct:
    // Although we might read a NPDTstruct12 in, we use NPDTstruct52 internally
    NPDTstruct52 mNpdt;

    int getFactionRank() const; /// wrapper for mNpdt*, -1 = no rank

    int mBloodType;
    unsigned char mFlags;

    InventoryList mInventory;
    SpellList mSpells;

    AIData mAiData;

    Transport mTransport;

    const std::vector<Transport::Dest>& getTransport() const;

    AIPackageList     mAiPackage;

    unsigned int mRecordFlags;
    std::string mId, mName, mModel, mRace, mClass, mFaction, mScript;

    // body parts
    std::string mHair, mHead;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    bool isMale() const;

    void setIsMale(bool value);

    void blank();
    ///< Set record to default state (does not touch the ID).

    /// Resets the mNpdt object
    void blankNpdt();
};
}
#endif
