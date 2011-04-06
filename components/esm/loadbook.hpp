#ifndef _ESM_BOOK_H
#define _ESM_BOOK_H

#include "esm_reader.hpp"

namespace ESM
{

/*
 * Books, magic scrolls, notes and so on
 */

struct Book
{
    struct BKDTstruct
    {
        float weight;
        int value, isScroll, skillID, enchant;
    };

    BKDTstruct data;
    std::string name, model, icon, script, enchant, text;

    void load(ESMReader &esm);
};
}
#endif
