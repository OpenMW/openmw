#include "npcstats.hpp"

#include <components/esm/loadnpc.hpp>
#include <components/esm/loadspel.hpp>

namespace CSMWorld
{
    NpcStats::NpcStats() : mHealth(0), mMana(0), mFatigue(0)
    {
        for (int i = 0; i < ESM::Skill::Length; ++i)
            mSkill[i] = 0;
    }

    NpcStats::NpcStats(const NpcStats &other)
    {
        for (int i = 0; i < ESM::Attribute::Length; ++i)
            mAttr[i] = other.mAttr[i];

        mSpells = other.mSpells;

        for (int i = 0; i < ESM::Skill::Length; ++i)
            mSkill[i] = 0;

        mHealth = other.mHealth;
        mMana = other.mMana;
        mFatigue = other.mFatigue;
    }

    NpcStats::~NpcStats()
    {}

    unsigned char NpcStats::getBaseAttribute(int index) const
    {
        if (index < 0 || index >= ESM::Attribute::Length)
            throw std::runtime_error("attrib index out of bounds");

        return mAttr[index];
    }

    void NpcStats::setAttribute(int index, unsigned char value)
    {
        if (index < 0 || index >= ESM::Attribute::Length)
            throw std::runtime_error("attrib index out of bounds");

        mAttr[index] = value;
    }

    void NpcStats::addSpell(const std::string& id)
    {
        struct SpellInfo info;
        info.mName = id;
        info.mType = ESM::Spell::ST_Spell; // default type from autocalc
        info.mFromRace = false;
        info.mCost = 0;
        info.mChance = 0;

        mSpells.insert(mSpells.begin(), info);
    }

    void NpcStats::addPowers(const std::string& id, int type)
    {
        struct SpellInfo info;
        info.mName = id;
        info.mType = type;
        info.mFromRace = true;
        info.mCost = 0;
        info.mChance = 0;

        mSpells.push_back(info);
    }

    void NpcStats::addCostAndChance(const std::string& id, int cost, int chance)
    {
        // usually only a few spells, so simply iterate through rather than keeping a separate
        // lookup index or map
        for (std::vector<SpellInfo>::iterator it = mSpells.begin(); it != mSpells.end(); ++it)
        {
            if ((*it).mName == id)
            {
                (*it).mCost = cost;
                (*it).mChance = chance;
                return;
            }
        }
    }

    const std::vector<SpellInfo>& NpcStats::spells() const
    {
        return mSpells;
    }

    unsigned char NpcStats::getBaseSkill(int index) const
    {
        if (index < 0 || index >= ESM::Skill::Length)
            throw std::runtime_error("skill index out of bounds");

        return mSkill[index];
    }

    void NpcStats::setBaseSkill(int index, unsigned char value)
    {
        if (index < 0 || index >= ESM::Skill::Length)
            throw std::runtime_error("skill index out of bounds");

        mSkill[index] = value;
    }

    unsigned short NpcStats::getHealth()
    {
        return mHealth;
    }

    void NpcStats::setHealth(unsigned short health)
    {
        mHealth = health;
    }

    unsigned short NpcStats::getMana()
    {
        return mMana;
    }

    void NpcStats::setMana(unsigned short mana)
    {
        mMana = mana;
    }

    unsigned short NpcStats::getFatigue()
    {
        return mFatigue;
    }

    void NpcStats::setFatigue(unsigned short fatigue)
    {
        mFatigue = fatigue;
    }
}

