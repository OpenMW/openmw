#include "loadbody.hpp"

namespace ESM
{

void BodyPart::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    name = esm.getHNString("FNAM");
    esm.getHNT(data, "BYDT", 4);
}

}
