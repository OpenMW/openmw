#include "loadclas.hpp"

#include <stdexcept>

#include "esmreader.hpp"
#include "esmwriter.hpp"
#include "components/esm/defs.hpp"

namespace ESM
{
    unsigned int Class::sRecordId = REC_CLAS;

    const Class::Specialization Class::sSpecializationIds[3] = {
      Class::Combat,
      Class::Magic,
      Class::Stealth
    };

    const char *Class::sGmstSpecializationIds[3] = {
      "sSpecializationCombat",
      "sSpecializationMagic",
      "sSpecializationStealth"
    };

    int& Class::CLDTstruct::getSkill (int index, bool major)
    {
        if (index<0 || index>=5)
            throw std::logic_error ("skill index out of range");

        return mSkills[index][major ? 1 : 0];
    }

    int Class::CLDTstruct::getSkill (int index, bool major) const
    {
        if (index<0 || index>=5)
            throw std::logic_error ("skill index out of range");

        return mSkills[index][major ? 1 : 0];
    }

    void Class::load(ESMReader &esm, bool &isDeleted)
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
                    mId = esm.getHString();
                    hasName = true;
                    break;
                case fourCC("FNAM"):
                    mName = esm.getHString();
                    break;
                case fourCC("CLDT"):
                    esm.getHTSized<60>(mData);
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
    void Class::save(ESMWriter &esm, bool isDeleted) const
    {
        esm.writeHNCString("NAME", mId);

        if (isDeleted)
        {
            esm.writeHNString("DELE", "", 3);
            return;
        }

        esm.writeHNOCString("FNAM", mName);
        esm.writeHNT("CLDT", mData, 60);
        esm.writeHNOString("DESC", mDescription);
    }

    void Class::blank()
    {
        mName.clear();
        mDescription.clear();

        mData.mAttribute[0] = mData.mAttribute[1] = 0;
        mData.mSpecialization = 0;
        mData.mIsPlayable = 0;
        mData.mCalc = 0;

        for (int i=0; i<5; ++i)
            for (int i2=0; i2<2; ++i2)
                mData.mSkills[i][i2] = 0;
    }
}
