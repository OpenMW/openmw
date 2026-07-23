#include "loadclas.hpp"

#include <stdexcept>

#include <components/esm/attr.hpp>
#include <components/esm/defs.hpp>
#include <components/esm3/loadskil.hpp>
#include <components/misc/concepts.hpp>

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    const std::string_view Class::sGmstSpecializationIds[3]
        = { "sSpecializationCombat", "sSpecializationMagic", "sSpecializationStealth" };
    const std::array<std::string_view, 3> Class::specializationIndexToLuaId = { "combat", "magic", "stealth" };

    namespace
    {
        struct EsmCLDTstruct
        {
            std::array<int32_t, 2> mAttribute;
            int32_t mSpecialization;
            std::array<std::array<int32_t, 2>, 5> mSkills;
            int32_t mIsPlayable;
            int32_t mServices;
        };

        void toBinary(const Class::CLDTstruct& src, EsmCLDTstruct& dst)
        {
            dst.mAttribute[0] = ESM::Attribute::refIdToIndex(src.mAttribute[0]);
            dst.mAttribute[1] = ESM::Attribute::refIdToIndex(src.mAttribute[1]);
            dst.mSpecialization = src.mSpecialization;
            for (std::size_t i = 0; i < src.mSkills.size(); ++i)
            {
                dst.mSkills[i][0] = ESM::Skill::refIdToIndex(src.mSkills[i][0]);
                dst.mSkills[i][1] = ESM::Skill::refIdToIndex(src.mSkills[i][1]);
            }
            dst.mIsPlayable = src.mIsPlayable;
            dst.mServices = src.mServices;
        }

        void fromBinary(const EsmCLDTstruct& src, Class::CLDTstruct& dst)
        {
            dst.mAttribute[0] = ESM::Attribute::indexToRefId(src.mAttribute[0]);
            dst.mAttribute[1] = ESM::Attribute::indexToRefId(src.mAttribute[1]);
            dst.mSpecialization = src.mSpecialization;
            for (std::size_t i = 0; i < src.mSkills.size(); ++i)
            {
                dst.mSkills[i][0] = ESM::Skill::indexToRefId(src.mSkills[i][0]);
                dst.mSkills[i][1] = ESM::Skill::indexToRefId(src.mSkills[i][1]);
            }
            dst.mIsPlayable = src.mIsPlayable;
            dst.mServices = src.mServices;
        }
    }

    template <Misc::SameAsWithoutCvref<EsmCLDTstruct> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mAttribute, v.mSpecialization, v.mSkills, v.mIsPlayable, v.mServices);
    }

    ESM::RefId& Class::CLDTstruct::getSkill(int index, bool major)
    {
        return mSkills.at(index)[major ? 1 : 0];
    }

    ESM::RefId Class::CLDTstruct::getSkill(int index, bool major) const
    {
        return mSkills.at(index)[major ? 1 : 0];
    }

    void Class::load(ESMReader& esm, bool& isDeleted)
    {
        isDeleted = false;
        mRecordFlags = esm.getRecordFlags();

        bool hasName = false;
        bool hasData = false;
        while (esm.hasMoreSubs())
        {
            esm.getSubName();
            switch (esm.retSubName().toInt())
            {
                case SREC_NAME:
                    mId = esm.getRefId();
                    hasName = true;
                    break;
                case fourCC("FNAM"):
                    mName = esm.getHString();
                    break;
                case fourCC("CLDT"):
                {
                    EsmCLDTstruct data;
                    esm.getSubComposite(data);
                    fromBinary(data, mData);
                    if (mData.mIsPlayable > 1)
                        esm.fail("Unknown bool value");
                    hasData = true;
                    break;
                }
                case fourCC("DESC"):
                    mDescription = esm.getHString();
                    break;
                case SREC_DELE:
                    esm.skipHSub();
                    isDeleted = true;
                    break;
                default:
                    esm.fail("Unknown subrecord");
                    break;
            }
        }

        if (!hasName)
            esm.fail("Missing NAME subrecord");
        if (!hasData && !isDeleted)
            esm.fail("Missing CLDT subrecord");
    }
    void Class::save(ESMWriter& esm, bool isDeleted) const
    {
        esm.writeHNCRefId("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNOCString("FNAM", mName);
        EsmCLDTstruct data;
        toBinary(mData, data);
        esm.writeNamedComposite("CLDT", data);
        esm.writeHNOString("DESC", mDescription);
    }

    void Class::blank()
    {
        mRecordFlags = 0;
        mName.clear();
        mDescription.clear();

        mData.mAttribute.fill({});
        mData.mSpecialization = 0;
        mData.mIsPlayable = 0;
        mData.mServices = 0;

        for (auto& skills : mData.mSkills)
            skills.fill({});
    }
}
