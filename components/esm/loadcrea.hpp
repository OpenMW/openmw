#ifndef OPENMW_ESM_CREA_H
#define OPENMW_ESM_CREA_H

#include <string>

#include "loadcont.hpp"
#include "spelllist.hpp"
#include "aipackage.hpp"

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

    // Default is 0x48?
    enum Flags
    {
        Biped       = 0x001,
        Respawn     = 0x002,
        Weapon      = 0x004, // Has weapon and shield
        None        = 0x008, // ??
        Swims       = 0x010,
        Flies       = 0x020, // Don't know what happens if several
        Walks       = 0x040, // of these are set
        Essential   = 0x080,
        Skeleton    = 0x400, // Does not have normal blood
        Metal       = 0x800  // Has 'golden' blood
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
        int mCombat, mMagic, mStealth; // Don't know yet.
        int mAttack[6]; // AttackMin1, AttackMax1, ditto2, ditto3
        int mGold;
    }; // 96 bytes

    NPDTstruct mData;

    int mFlags;

    bool mPersistent;

    float mScale;

    std::string mId, mModel, mName, mScript;
    std::string mOriginal; // Base creature that this is a modification of

    InventoryList mInventory;
    SpellList mSpells;


    bool mHasAI;
    AIData mAiData;
    AIPackageList mAiPackage;

    void load(ESMReader &esm);
    void save(ESMWriter &esm) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};

}
#endif
