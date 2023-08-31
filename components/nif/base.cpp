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

    void Named::read(NIFStream* nif)
    {
        name = nif->getString();
        if (nif->getVersion() < NIFStream::generateVersion(10, 0, 1, 0))
            extra.read(nif);
        else
            readRecordList(nif, extralist);
        controller.read(nif);
    }

    void Named::post(Reader& nif)
    {
        extra.post(nif);
        postRecordList(nif, extralist);
        controller.post(nif);
    }
}
