#include "loadclas.hpp"

#include <stdexcept>

#include <components/esm/defs.hpp>
#include <components/misc/concepts.hpp>

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{
    const std::string_view Class::sGmstSpecializationIds[3]
        = { "sSpecializationCombat", "sSpecializationMagic", "sSpecializationStealth" };
    const std::array<std::string_view, 3> Class::specializationIndexToLuaId = { "combat", "magic", "stealth" };

    template <Misc::SameAsWithoutCvref<Class::CLDTstruct> T>
    void decompose(T&& v, const auto& f)
    {
        f(v.mAttribute, v.mSpecialization, v.mSkills, v.mIsPlayable, v.mServices);
    }

    int32_t& Class::CLDTstruct::getSkill(int index, bool major)
    {
        return mSkills.at(index)[major ? 1 : 0];
    }

    int32_t Class::CLDTstruct::getSkill(int index, bool major) const
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
                    esm.getSubComposite(mData);
                    if (mData.mIsPlayable > 1)
                        esm.fail("Unknown bool value");
                    hasData = true;
                    break;
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
        esm.writeNamedComposite("CLDT", mData);
        esm.writeHNOString("DESC", mDescription);
    }

    void Class::blank()
    {
        mRecordFlags = 0;
        mName.clear();
        mDescription.clear();

        mData.mAttribute.fill(0);
        mData.mSpecialization = 0;
        mData.mIsPlayable = 0;
        mData.mServices = 0;

        for (auto& skills : mData.mSkills)
            skills.fill(0);
    }
}
