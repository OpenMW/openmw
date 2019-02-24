#ifndef OPENMW_ESM_CREA_H
#define OPENMW_ESM_CREA_H

#include <string>

#include "loadcont.hpp"
#include "spelllist.hpp"
#include "aipackage.hpp"
#include "transport.hpp"

namespace ESM
{

class ESMReader;
class ESMWriter;

/*
 * Creature definition
 *
 */

struct Creature
{
    static unsigned int sRecordId;
    /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
    static std::string getRecordType() { return "Creature"; }

    // Default is 0x48?
    enum Flags
    {
        // Movement types
        Bipedal       = 0x001,
        Swims       = 0x010,
        Flies       = 0x020, // Don't know what happens if several
        Walks       = 0x040, // of these are set

        Respawn     = 0x002,
        Weapon      = 0x004, // Has weapon and shield
        None        = 0x008, // ?? This flag appears set for every creature in Morrowind.esm
        Essential   = 0x080,

        // Blood types
        Skeleton    = 0x400,
        Metal       = 0x800
    };

    enum Type
    {
        Creatures = 0,
        Daedra  = 1,
        Undead = 2,
        Humanoid = 3
    };

    struct NPDTstruct
    {
        int mType;
        // For creatures we obviously have to use ints, not shorts and
        // bytes like we use for NPCs.... this file format just makes so
        // much sense! (Still, _much_ easier to decode than the NIFs.)
        int mLevel;
        int mStrength,
            mIntelligence,
            mWillpower,
            mAgility,
            mSpeed,
            mEndurance,
            mPersonality,
            mLuck;

        int mHealth, mMana, mFatigue; // Stats
        int mSoul; // The creatures soul value (used with soul gems.)
        // Creatures have generalized combat, magic and stealth stats which substitute for
        // the specific skills (in the same way as specializations).
        int mCombat, mMagic, mStealth;
        int mAttack[6]; // AttackMin1, AttackMax1, ditto2, ditto3
        int mGold;
    }; // 96 byte

    NPDTstruct mData;

    int mFlags;

    bool mPersistent;

    float mScale;

    std::string mId, mModel, mName, mScript;
    std::string mOriginal; // Base creature that this is a modification of

    InventoryList mInventory;
    SpellList mSpells;

    AIData mAiData;
    AIPackageList mAiPackage;
    Transport mTransport;

    const std::vector<Transport::Dest>& getTransport() const;

    void load(ESMReader &esm, bool &isDeleted);
    void save(ESMWriter &esm, bool isDeleted = false) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};

}
#endif
