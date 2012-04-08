#ifndef _ESM_BOOK_H
#define _ESM_BOOK_H

#include "record.hpp"
#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM
{

/*
 * Books, magic scrolls, notes and so on
 */

struct Book : public Record
{
    struct BKDTstruct
    {
        float weight;
        int value, isScroll, skillID, enchant;
    };

    BKDTstruct data;
    std::string name, model, icon, script, enchant, text;

    void load(ESMReader &esm);
    void save(ESMWriter &esm);
    
    int getName() { return REC_BOOK; }
};
}
#endif
