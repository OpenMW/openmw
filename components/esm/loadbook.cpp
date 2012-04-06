#include "loadbook.hpp"

namespace ESM
{

void Book::load(ESMReader &esm)
{
    model = esm.getHNString("MODL");
    name = esm.getHNOString("FNAM");
    esm.getHNT(data, "BKDT", 20);
    script = esm.getHNOString("SCRI");
    icon = esm.getHNOString("ITEX");
    text = esm.getHNOString("TEXT");
    enchant = esm.getHNOString("ENAM");
}
void Book::save(ESMWriter &esm)
{
    esm.writeHNString("MODL", model);
    esm.writeHNOString("FNAM", name);
    esm.writeHNT("BKDT", data, 20);
    esm.writeHNOString("SCRI", script);
    esm.writeHNOString("ITEX", icon);
    esm.writeHNOString("TEXT", text);
    esm.writeHNOString("ENAM", enchant);
}

}
