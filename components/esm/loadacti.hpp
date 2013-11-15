#ifndef OPENMW_ESM_ACTI_H
#define OPENMW_ESM_ACTI_H

#include <string>

namespace ESM
{

class ESMReader;
class ESMWriter;

struct Activator
{
    static unsigned int sRecordId;

    std::string mId, mName, mScript, mModel;

    void load(ESMReader &esm);
    void save(ESMWriter &esm) const;

    void blank();
    ///< Set record to default state (does not touch the ID).
};

}
#endif
