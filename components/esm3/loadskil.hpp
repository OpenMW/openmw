#ifndef OPENMW_ESM_SKIL_H
#define OPENMW_ESM_SKIL_H

#include <array>
#include <string>

#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     * Skill information
     *
     */

    struct Skill
    {

        constexpr static RecNameInts sRecordId = REC_SKIL;
        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "Skill"; }

        unsigned int mRecordFlags;
        RefId mId;

        struct SKDTstruct
        {
            int mAttribute; // see defs.hpp
            int mSpecialization; // 0 - Combat, 1 - Magic, 2 - Stealth
            float mUseValue[4]; // How much skill improves through use. Meaning
                                // of each field depends on what skill this
                                // is. We should document this better later.
        }; // Total size: 24 bytes
        SKDTstruct mData;

        // Skill index. Skils don't have an id ("NAME") like most records,
        // they only have a numerical index that matches one of the
        // hard-coded skills in the game.
        int mIndex{ -1 };

        std::string mDescription;
        std::string mName;
        std::string mIcon;
        float mWerewolfValue{};

        static const RefId Block;
        static const RefId Armorer;
        static const RefId MediumArmor;
        static const RefId HeavyArmor;
        static const RefId BluntWeapon;
        static const RefId LongBlade;
        static const RefId Axe;
        static const RefId Spear;
        static const RefId Athletics;
        static const RefId Enchant;
        static const RefId Destruction;
        static const RefId Alteration;
        static const RefId Illusion;
        static const RefId Conjuration;
        static const RefId Mysticism;
        static const RefId Restoration;
        static const RefId Alchemy;
        static const RefId Unarmored;
        static const RefId Security;
        static const RefId Sneak;
        static const RefId Acrobatics;
        static const RefId LightArmor;
        static const RefId ShortBlade;
        static const RefId Marksman;
        static const RefId Mercantile;
        static const RefId Speechcraft;
        static const RefId HandToHand;
        static constexpr int Length = 27;
        static const std::string sSkillNames[Length];

        static int stringToSkillId(std::string_view skill);

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        void blank();
        ///< Set record to default state (does not touch the ID/index).

        static RefId indexToRefId(int index);
    };
}
#endif
