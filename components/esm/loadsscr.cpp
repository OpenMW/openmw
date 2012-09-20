#include "loadsscr.hpp"

#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

void StartScript::load(ESMReader &esm)
{
    mData = esm.getHNString("DATA");
    mScript = esm.getHNString("NAME");
}
void StartScript::save(ESMWriter &esm)
{
    esm.writeHNString("DATA", mData);
    esm.writeHNString("NAME", mScript);
}

}
