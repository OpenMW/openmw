#include "loadstat.hpp"

#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

void Static::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
}
void Static::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", mModel);
}

}
