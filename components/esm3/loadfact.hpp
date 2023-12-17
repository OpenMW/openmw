#ifndef OPENMW_ESM_FACT_H
#define OPENMW_ESM_FACT_H

#include <array>
#include <map>
#include <string>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     * Faction definitions
     */

    // Requirements for each rank
    struct RankData
    {
        int32_t mAttribute1, mAttribute2; // Attribute level

        // Skill level (faction skills given in
        // skillID below.) You need one skill at
        // level 'mPrimarySkill' and two skills at level
        // 'mFavouredSkill' to advance to this rank.
        int32_t mPrimarySkill, mFavouredSkill;

        int32_t mFactReaction; // Reaction from faction members

        void load(ESMReader& esm);
        void save(ESMWriter& esm) const;
    };

    struct Faction
    {
        constexpr static RecNameInts sRecordId = REC_FACT;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Faction"; }

        uint32_t mRecordFlags;
        std::string mName;
        RefId mId;

        struct FADTstruct
        {
            // Which attributes we like
            std::array<int32_t, 2> mAttribute;

            std::array<RankData, 10> mRankData;

            std::array<int32_t, 7> mSkills; // IDs of skills this faction require
                                            // Each element will either contain an Skill index, or -1.

            int32_t mIsHidden; // 1 - hidden from player

            int32_t& getSkill(size_t index, bool ignored = false);
            ///< Throws an exception for invalid values of \a index.

            int32_t getSkill(size_t index, bool ignored = false) const;
            ///< Throws an exception for invalid values of \a index.

            void load(ESMReader& esm);
            void save(ESMWriter& esm) const;
        }; // 240 bytes

        FADTstruct mData;

        // <Faction ID, Reaction>
        std::map<ESM::RefId, int32_t> mReactions;

        // Name of faction ranks (may be empty for NPC factions)
        std::array<std::string, 10> mRanks;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID/index).
    };
}
#endif
