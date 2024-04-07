#ifndef OPENMW_ESM_NPC_H
#define OPENMW_ESM_NPC_H

#include <array>
#include <string>
#include <vector>

#include "aipackage.hpp"
#include "components/esm/attr.hpp"
#include "components/esm/defs.hpp"
#include "components/esm/refid.hpp"
#include "loadcont.hpp"
#include "loadskil.hpp"
#include "spelllist.hpp"
#include "transport.hpp"

namespace ESM
{

    class ESMReader;
    class ESMWriter;

    /*
     * NPC definition
     */

    struct NPC
    {
        constexpr static RecNameInts sRecordId = REC_NPC_;

        /// Return a string descriptor for this record type. Currently used for debugging / error logs only.
        static std::string_view getRecordType() { return "NPC"; }

        // Services
        enum Services
        {
            // This merchant buys:
            Weapon = 0x00001,
            Armor = 0x00002,
            Clothing = 0x00004,
            Books = 0x00008,
            Ingredients = 0x00010,
            Picks = 0x00020,
            Probes = 0x00040,
            Lights = 0x00080,
            Apparatus = 0x00100,
            RepairItem = 0x00200,
            Misc = 0x00400,
            Potions = 0x02000,

            AllItems = Weapon | Armor | Clothing | Books | Ingredients | Picks | Probes | Lights | Apparatus
                | RepairItem | Misc | Potions,

            // Other services
            Spells = 0x00800,
            MagicItems = 0x01000,
            Training = 0x04000,
            Spellmaking = 0x08000,
            Enchanting = 0x10000,
            Repair = 0x20000
        };

        enum Flags
        {
            Female = 0x01,
            Essential = 0x02,
            Respawn = 0x04,
            Base = 0x08,
            Autocalc = 0x10
        };

        enum NpcType
        {
            NPC_WITH_AUTOCALCULATED_STATS = 12,
            NPC_DEFAULT = 52
        };

        struct NPDTstruct52
        {
            int16_t mLevel;
            std::array<unsigned char, Attribute::Length> mAttributes;

            // mSkill can grow up to 200, it must be unsigned
            std::array<unsigned char, Skill::Length> mSkills;

            uint16_t mHealth, mMana, mFatigue;
            unsigned char mDisposition, mReputation, mRank;
            int32_t mGold;
        }; // 52 bytes

        unsigned char mNpdtType;
        // Worth noting when saving the struct:
        //  Although we might read a NPDTstruct12 in, we use NPDTstruct52 internally
        NPDTstruct52 mNpdt;

        int getFactionRank() const; /// wrapper for mNpdt*, -1 = no rank

        int32_t mBloodType;
        unsigned char mFlags;

        InventoryList mInventory;
        SpellList mSpells;

        AIData mAiData;

        Transport mTransport;

        const std::vector<Transport::Dest>& getTransport() const;

        AIPackageList mAiPackage;

        uint32_t mRecordFlags;
        RefId mId, mRace, mClass, mFaction, mScript;
        std::string mModel, mName;

        // body parts
        RefId mHair, mHead;

        void load(ESMReader& esm, bool& isDeleted);
        void save(ESMWriter& esm, bool isDeleted = false) const;

        bool isMale() const;

        void setIsMale(bool value);

        void blank();
        ///< Set record to default state (does not touch the ID).

        /// Resets the mNpdt object
        void blankNpdt();
    };
}
#endif
