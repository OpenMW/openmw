#include "loadsscr.hpp"

namespace ESM
{

void StartScript::load(ESMReader &esm)
{
    esm.getSubNameIs("DATA");
    esm.skipHSub();
    script = esm.getHNString("NAME");
}
void StartScript::save(ESMWriter &esm)
{
    esm.writeHNString("DATA", "NIET");
    esm.writeHNString("NAME", script);
}

}
