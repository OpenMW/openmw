#ifndef COMPONENTS_GAMEPLAY_AUTOCALC_H
#define COMPONENTS_GAMEPLAY_AUTOCALC_H

#include <string>

#include "store.hpp"

namespace ESM
{
    struct NPC;
    struct Race;
    struct Class;
}

namespace GamePlay
{
    // wrapper class for sharing the autocalc code between OpenMW and OpenCS
    class  StatsBase
    {

    public:

        StatsBase();
        virtual ~StatsBase();

        virtual unsigned char getBaseAttribute(int index) const = 0;

        virtual void setAttribute(int index, unsigned char value) = 0;

        virtual void addSpells(std::string id) = 0;

        virtual unsigned char getBaseSkill(int index) const = 0;

        virtual void setBaseSkill(int index, unsigned char value) = 0;
    };

    void autoCalcAttributesImpl (const ESM::NPC* npc,
            const ESM::Race *race, const ESM::Class *class_, int level, StatsBase& stats, StoreWrap *store);

    void autoCalcSkillsImpl (const ESM::NPC* npc,
            const ESM::Race *race, const ESM::Class *class_, int level, StatsBase& stats, StoreWrap *store);

    unsigned short autoCalculateHealth(int level, const ESM::Class *class_, StatsBase& stats);

    void autoCalculateSpells(const ESM::Race *race, StatsBase& stats, StoreWrap *store);
}
#endif // COMPONENTS_GAMEPLAY_AUTOCALC_H
