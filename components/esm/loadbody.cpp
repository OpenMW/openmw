#include "loadbody.hpp"

namespace ESM
{

void BodyPart::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    name = esm.getHNString("FNAM");
    esm.getHNT(data, "BYDT", 4);
}
void BodyPart::save(ESMWriter &esm)
{
    esm.writeHNString("MODL", model);
    esm.writeHNString("FNAM", name);
    esm.writeHNT("BYDT", data, 4);
}

}
