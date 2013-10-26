#include "effectlist.hpp"

#include "esmreader.hpp"
#include "esmwriter.hpp"

namespace ESM {

void EffectList::load(ESMReader &esm)
{
    ENAMstruct s;
    while (esm.isNextSub("ENAM")) {
        esm.getHT(s, 24);
        mList.push_back(s);
    }
}

void EffectList::save(ESMWriter &esm) const
{
    for (std::vector<ENAMstruct>::const_iterator it = mList.begin(); it != mList.end(); ++it) {
        esm.writeHNT<ENAMstruct>("ENAM", *it, 24);
    }
}

} // end namespace
