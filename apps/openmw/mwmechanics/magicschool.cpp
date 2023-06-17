#include "magicschool.hpp"

#include <components/esm3/loadgmst.hpp>

#include "../mwworld/esmstore.hpp"

#include "../mwbase/environment.hpp"

namespace
{
    std::array<MWMechanics::MagicSchool, MWMechanics::MagicSchool::Length> initSchools()
    {
        const MWWorld::Store<ESM::GameSetting>& gmst
            = MWBase::Environment::get().getESMStore()->get<ESM::GameSetting>();
        std::array<MWMechanics::MagicSchool, MWMechanics::MagicSchool::Length> out;
        const std::string schools[]
            = { "alteration", "conjuration", "destruction", "illusion", "mysticism", "restoration" };
        for (size_t i = 0; i < out.size(); ++i)
        {
            out[i].mAreaSound = ESM::RefId::stringRefId(schools[i] + " area");
            out[i].mBoltSound = ESM::RefId::stringRefId(schools[i] + " bolt");
            out[i].mCastSound = ESM::RefId::stringRefId(schools[i] + " cast");
            out[i].mFailureSound = ESM::RefId::stringRefId("Spell Failure " + schools[i]);
            out[i].mHitSound = ESM::RefId::stringRefId(schools[i] + " hit");
            out[i].mName = gmst.find("sSchool" + schools[i])->mValue.getString();
            out[i].mAutoCalcMax = gmst.find("iAutoSpell" + schools[i] + "Max")->mValue.getInteger();
        }
        return out;
    }
}

namespace MWMechanics
{
    const MagicSchool& getMagicSchool(int index)
    {
        static const std::array<MagicSchool, MagicSchool::Length> sSchools = initSchools();
        return sSchools[index];
    }
}
