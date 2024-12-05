#include "base.hpp"

namespace Nif
{
    void Extra::read(NIFStream* nif)
    {
        if (nif->getVersion() >= NIFStream::generateVersion(10, 0, 1, 0))
            nif->read(mName);
        else if (nif->getVersion() <= NIFStream::generateVersion(4, 2, 2, 0))
        {
            mNext.read(nif);
            nif->read(mRecordSize);
        }
    }

    void NiObjectNET::read(NIFStream* nif)
    {
        nif->read(mName);
        if (nif->getVersion() < NIFStream::generateVersion(10, 0, 1, 0))
            mExtra.read(nif);
        else
            readRecordList(nif, mExtraList);
        mController.read(nif);
    }

    void NiObjectNET::post(Reader& nif)
    {
        mExtra.post(nif);
        postRecordList(nif, mExtraList);
        mController.post(nif);
    }

    ExtraList NiObjectNET::getExtraList() const
    {
        ExtraList list = mExtraList;
        for (ExtraPtr extra = mExtra; !extra.empty(); extra = extra->mNext)
            list.emplace_back(extra);
        return list;
    }
}
