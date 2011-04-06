#ifndef _ESM_APPA_H
#define _ESM_APPA_H

#include "esm_reader.hpp"

namespace ESM
{

/*
 * Alchemist apparatus
 */

struct Apparatus
{
    enum AppaType
    {
        MortarPestle = 0, Albemic = 1, Calcinator = 2, Retort = 3
    };

    struct AADTstruct
    {
        int type;
        float quality;
        float weight;
        int value;
    };

    AADTstruct data;
    std::string model, icon, script, name;

    void load(ESMReader &esm);
};
}
#endif
