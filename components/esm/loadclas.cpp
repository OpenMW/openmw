#include "loadclas.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM
{

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

void Class::load(ESMReader &esm)
{
    mName = esm.getHNString("FNAM");
    esm.getHNT(mData, "CLDT", 60);

    if (mData.mIsPlayable > 1)
        esm.fail("Unknown bool value");

    mDescription = esm.getHNOString("DESC");
}
void Class::save(ESMWriter &esm)
{
    esm.writeHNCString("FNAM", mName);
    esm.writeHNT("CLDT", mData, 60);
    esm.writeHNOString("DESC", mDescription);
}

}
