#ifndef OPENMW_ESM_EFFECTLIST_H
#define OPENMW_ESM_EFFECTLIST_H

#include <vector>

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    #pragma pack(push)
    #pragma pack(1)

    /** Defines a spell effect. Shared between SPEL (Spells), ALCH
     (Potions) and ENCH (Item enchantments) records
     */
    struct ENAMstruct
    {
        // Magical effect, hard-coded ID
        short mEffectID;

        // Which skills/attributes are affected (for restore/drain spells
        // etc.)
        signed char mSkill, mAttribute; // -1 if N/A

        // Other spell parameters
        int mRange; // 0 - self, 1 - touch, 2 - target (RangeType enum)
        int mArea, mDuration, mMagnMin, mMagnMax;
    };
    #pragma pack(pop)

    /// EffectList, ENAM subrecord
    struct EffectList
    {
        std::vector<ENAMstruct> mList;

        /// Load one effect, assumes subrecord name was already read
        void add(ESMReader &esm);

        /// Load all effects
        void load(ESMReader &esm);
        void save(ESMWriter &esm) const;
    };

}

#endif
