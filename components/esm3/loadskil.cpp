#include "loadskil.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

#include <components/esm/attr.hpp>
#include <components/misc/concepts.hpp>
#include <components/misc/strings/algorithm.hpp>

#include <cstdint>

namespace ESM
{
    const RefId Skill::Block(ESM::StringRefId("Block"));
    const RefId Skill::Armorer(ESM::StringRefId("Armorer"));
    const RefId Skill::MediumArmor(ESM::StringRefId("MediumArmor"));
    const RefId Skill::HeavyArmor(ESM::StringRefId("HeavyArmor"));
    const RefId Skill::BluntWeapon(ESM::StringRefId("BluntWeapon"));
    const RefId Skill::LongBlade(ESM::StringRefId("LongBlade"));
    const RefId Skill::Axe(ESM::StringRefId("Axe"));
    const RefId Skill::Spear(ESM::StringRefId("Spear"));
    const RefId Skill::Athletics(ESM::StringRefId("Athletics"));
    const RefId Skill::Enchant(ESM::StringRefId("Enchant"));
    const RefId Skill::Destruction(ESM::StringRefId("Destruction"));
    const RefId Skill::Alteration(ESM::StringRefId("Alteration"));
    const RefId Skill::Illusion(ESM::StringRefId("Illusion"));
    const RefId Skill::Conjuration(ESM::StringRefId("Conjuration"));
    const RefId Skill::Mysticism(ESM::StringRefId("Mysticism"));
    const RefId Skill::Restoration(ESM::StringRefId("Restoration"));
    const RefId Skill::Alchemy(ESM::StringRefId("Alchemy"));
    const RefId Skill::Unarmored(ESM::StringRefId("Unarmored"));
    const RefId Skill::Security(ESM::StringRefId("Security"));
    const RefId Skill::Sneak(ESM::StringRefId("Sneak"));
    const RefId Skill::Acrobatics(ESM::StringRefId("Acrobatics"));
    const RefId Skill::LightArmor(ESM::StringRefId("LightArmor"));
    const RefId Skill::ShortBlade(ESM::StringRefId("ShortBlade"));
    const RefId Skill::Marksman(ESM::StringRefId("Marksman"));
    const RefId Skill::Mercantile(ESM::StringRefId("Mercantile"));
    const RefId Skill::Speechcraft(ESM::StringRefId("Speechcraft"));
    const RefId Skill::HandToHand(ESM::StringRefId("HandToHand"));

    namespace
    {
        struct EsmSKDTstruct
        {
            int32_t mAttribute;
            int32_t mSpecialization;
            float mUseValue[4];
        };

        void toBinary(const Skill::SKDTstruct& src, EsmSKDTstruct& dst)
        {
            dst.mAttribute = ESM::Attribute::refIdToIndex(src.mAttribute);
            dst.mSpecialization = src.mSpecialization;
            for (std::size_t i = 0; i < std::size(dst.mUseValue); ++i)
                dst.mUseValue[i] = src.mUseValue[i];
        }

        void fromBinary(const EsmSKDTstruct& src, Skill::SKDTstruct& dst)
        {
            dst.mAttribute = ESM::Attribute::indexToRefId(src.mAttribute);
            dst.mSpecialization = src.mSpecialization;
            for (std::size_t i = 0; i < std::size(dst.mUseValue); ++i)
                dst.mUseValue[i] = src.mUseValue[i];
        }
    }

    template <Misc::SameAsWithoutCvref<EsmSKDTstruct> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mAttribute, v.mSpecialization, v.mUseValue);
    }

    void Skill::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false; // Skill record can't be deleted now (may be changed in the future)
        mRecordFlags = esm.getRecordFlags();

        bool hasIndex = false;
        bool hasData = false;
        int32_t index = -1;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case fourCC("INDX"):
                    esm.getHT(index);
                    hasIndex = true;
                    break;
                case fourCC("SKDT"):
                {
                    EsmSKDTstruct data;
                    esm.getSubComposite(data);
                    hasData = true;
                    fromBinary(data, mData);
                    break;
                }
                case fourCC("DESC"):
                    mDescription = esm.getHString();
                    break;
                default:
                    esm.fail("Unknown subrecord");
            }
        }
        if (!hasIndex)
            esm.fail("Missing INDX");
        else if (index < 0 || index >= Length)
            esm.fail("Invalid INDX");
        if (!hasData)
            esm.fail("Missing SKDT");

        mId = indexToRefId(index);
    }

    void Skill::save(ESMWriter& esm, bool /*isDeleted*/) const
    {
        esm.writeHNT("INDX", refIdToIndex(mId));
        EsmSKDTstruct data;
        toBinary(mData, data);
        esm.writeNamedComposite("SKDT", data);
        esm.writeHNOString("DESC", mDescription);
    }

    void Skill::blank()
    {
        mRecordFlags = 0;
        mData.mAttribute = {};
        mData.mSpecialization = 0;
        mData.mUseValue.fill(1.f);
        mDescription.clear();
    }

    static const RefId sSkills[Skill::Length] = {
        Skill::Block,
        Skill::Armorer,
        Skill::MediumArmor,
        Skill::HeavyArmor,
        Skill::BluntWeapon,
        Skill::LongBlade,
        Skill::Axe,
        Skill::Spear,
        Skill::Athletics,
        Skill::Enchant,
        Skill::Destruction,
        Skill::Alteration,
        Skill::Illusion,
        Skill::Conjuration,
        Skill::Mysticism,
        Skill::Restoration,
        Skill::Alchemy,
        Skill::Unarmored,
        Skill::Security,
        Skill::Sneak,
        Skill::Acrobatics,
        Skill::LightArmor,
        Skill::ShortBlade,
        Skill::Marksman,
        Skill::Mercantile,
        Skill::Speechcraft,
        Skill::HandToHand,
    };

    RefId Skill::indexToRefId(int index)
    {
        if (index < 0 || index >= Length)
            return RefId();
        return sSkills[index];
    }

    int Skill::refIdToIndex(RefId id)
    {
        for (int i = 0; i < Length; ++i)
        {
            if (sSkills[i] == id)
                return i;
        }
        return -1;
    }

    const std::array<RefId, MagicSchool::Length> sMagicSchools = {
        Skill::Alteration,
        Skill::Conjuration,
        Skill::Destruction,
        Skill::Illusion,
        Skill::Mysticism,
        Skill::Restoration,
    };

    RefId MagicSchool::indexToSkillRefId(int index)
    {
        if (index < 0 || index >= Length)
            return {};
        return sMagicSchools[index];
    }

    int MagicSchool::skillRefIdToIndex(RefId id)
    {
        for (size_t i = 0; i < sMagicSchools.size(); ++i)
        {
            if (id == sMagicSchools[i])
                return static_cast<int>(i);
        }
        return -1;
    }
}
