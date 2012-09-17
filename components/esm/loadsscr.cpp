#include "loadsscr.hpp"

#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

void StartScript::load(ESMReader &esm)
{
    esm.getSubNameIs("DATA");
    esm.skipHSub();
    mScript = esm.getHNString("NAME");
}
void StartScript::save(ESMWriter &esm)
{
    esm.writeHNString("DATA", "NIET");
    esm.writeHNString("NAME", mScript);
}

}
