#include "loadclas.hpp"

namespace ESM
{

const Class::Specialization Class::specializationIds[3] = {
  Class::Combat,
  Class::Magic,
  Class::Stealth
};

const char *Class::gmstSpecializationIds[3] = {
  "sSpecializationCombat",
  "sSpecializationMagic",
  "sSpecializationStealth"
};

void Class::load(ESMReader &esm)
{
    name = esm.getHNString("FNAM");
    esm.getHNT(data, "CLDT", 60);

    if (data.isPlayable > 1)
        esm.fail("Unknown bool value");

    description = esm.getHNOString("DESC");
}
void Class::save(ESMWriter &esm)
{
    esm.writeHNString("FNAM", name);
    esm.writeHNT("CLDT", data, 60);
    esm.writeHNOString("DESC", description);
}

}
