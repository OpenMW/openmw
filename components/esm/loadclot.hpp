#ifndef _ESM_CLOT_H
#define _ESM_CLOT_H

#include "esm_reader.hpp"
#include "esm_writer.hpp"
#include "loadarmo.hpp"

namespace ESM
{

/*
 * Clothing
 */

struct Clothing
{
    enum Type
    {
        Pants = 0,
        Shoes = 1,
        Shirt = 2,
        Belt = 3,
        Robe = 4,
        RGlove = 5,
        LGlove = 6,
        Skirt = 7,
        Ring = 8,
        Amulet = 9
    };

    struct CTDTstruct
    {
        int type;
        float weight;
        short value;
        short enchant;
    };
    CTDTstruct data;

    PartReferenceList parts;

    std::string name, model, icon, enchant, script;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
};
}
#endif
