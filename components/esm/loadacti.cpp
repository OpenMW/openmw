#include "loadacti.hpp"

#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{
void Activator::load(ESMReader &esm)
{
    mModel = esm.getHNString("MODL");
    mName = esm.getHNString("FNAM");
    mScript = esm.getHNOString("SCRI");
}
void Activator::save(ESMWriter &esm)
{
    esm.writeHNCString("MODL", mModel);
    esm.writeHNCString("FNAM", mName);
    esm.writeHNOCString("SCRI", mScript);
}
}
