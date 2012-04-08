#ifndef _ESM_CREC_H
#define _ESM_CREC_H

#include "record.hpp"
#include "esm_reader.hpp"
#include "esm_writer.hpp"

namespace ESM {

/* These two are only used in save games. They are not decoded yet.
 */

/// Changes a creature
struct LoadCREC : public Record
{
    void load(ESMReader &esm)
    {
      esm.skipRecord();
    }

    void save(ESMWriter &esm)
    {
    }

    int getName() { return REC_CREC; }
};

/// Changes an item list / container
struct LoadCNTC : public Record
{
    void load(ESMReader &esm)
    {
      esm.skipRecord();
    }

    void save(ESMWriter &esm)
    {
    }

    int getName() { return REC_CNTC; }
};
}
#endif
