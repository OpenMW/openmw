#include "loadstat.hpp"

namespace ESM
{

void Static::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
}

}
