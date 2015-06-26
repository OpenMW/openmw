#ifndef COMPONENTS_AUTOCALC_STORE_H
#define COMPONENTS_AUTOCALC_STORE_H

#include <vector>
#include <string>

namespace ESM
{
    struct Spell;
    struct Skill;
    struct MagicEffect;
}

namespace AutoCalc
{
    // interface class for sharing the autocalc component between OpenMW and OpenCS
    class StoreCommon
    {

    public:
        StoreCommon() {}
        virtual ~StoreCommon() {}

        virtual int findGmstInt(const std::string& gmst) const = 0;

        virtual float findGmstFloat(const std::string& gmst) const = 0;

        virtual const ESM::Skill *findSkill(int index) const = 0;

        virtual const ESM::MagicEffect* findMagicEffect(int id) const = 0;

        virtual void getSpells(std::vector<ESM::Spell*>& spells) = 0;
    };
}
#endif // COMPONENTS_AUTOCALC_STORE_H
