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
    if (!name.empty())
        esm.writeHNString("FNAM", name);
    esm.writeHNT("BKDT", data, 20);
    if (!script.empty())
        esm.writeHNString("SCRI", script);
    if (!icon.empty())
        esm.writeHNString("ITEX", icon);
    if (!text.empty())
        esm.writeHNString("TEXT", text);
    if (!enchant.empty())
        esm.writeHNString("ENAM", enchant);
}

}
