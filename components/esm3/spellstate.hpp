#ifndef OPENMW_ESM_SPELLSTATE_H
#define OPENMW_ESM_SPELLSTATE_H

#include <map>
#include <vector>
#include <string>
#include <set>

#include "components/esm/defs.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // NOTE: spell ids must be lower case
    struct SpellState
    {
        struct CorprusStats
        {
            int mWorsenings;
            TimeStamp mNextWorsening;
        };

        struct PermanentSpellEffectInfo
        {
            int mId;
            int mArg;
            float mMagnitude;
        };

        struct SpellParams
        {
            std::map<int, float> mEffectRands; // <effect index, normalised random magnitude>
            std::set<int> mPurgedEffects; // indices of purged effects
        };
        std::vector<std::string> mSpells;

        // FIXME: obsolete, used only for old saves
        std::map<std::string, SpellParams> mSpellParams;
        std::map<std::string, std::vector<PermanentSpellEffectInfo> > mPermanentSpellEffects;
        std::map<std::string, CorprusStats> mCorprusSpells;

        std::map<std::string, TimeStamp> mUsedPowers;

        std::string mSelectedSpell;

        void load (ESMReader &esm);
        void save (ESMWriter &esm) const;
    };

}

#endif
