#ifndef CSM_WORLD_NPCSTATS_H
#define CSM_WORLD_NPCSTATS_H

#include <vector>

#include <QMetaType>

#include <components/esm/attr.hpp>
#include <components/esm/loadskil.hpp>
#include <components/autocalc/autocalc.hpp>

namespace CSMWorld
{
    struct SpellInfo
    {
        std::string mName;
        int mType;
        bool mFromRace;
        int mCost;
        int mChance;
    };

    class NpcStats : public AutoCalc::StatsBase
    {

        int mAttr[ESM::Attribute::Length];
        std::vector<SpellInfo> mSpells;
        int mSkill[ESM::Skill::Length];

        unsigned short mHealth;
        unsigned short mMana;
        unsigned short mFatigue;

    public:

        NpcStats();

        NpcStats(const NpcStats &other);

        ~NpcStats();

        virtual unsigned char getBaseAttribute(int index) const;

        virtual void setAttribute(int index, unsigned char value);

        virtual void addSpell(const std::string& id);

        void addPowers(const std::string& id, int type);

        void addCostAndChance(const std::string& id, int cost, int chance);

        const std::vector<SpellInfo>& spells() const;

        virtual unsigned char getBaseSkill(int index) const;

        virtual void setBaseSkill(int index, unsigned char value);

        unsigned short getHealth();

        void setHealth(unsigned short health);

        unsigned short getMana();

        void setMana(unsigned short mana);

        unsigned short getFatigue();

        void setFatigue(unsigned short fatigue);
    };
}

Q_DECLARE_METATYPE(CSMWorld::NpcStats*)

#endif // CSM_WORLD_NPCSTATS_H
