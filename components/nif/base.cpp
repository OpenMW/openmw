#include "base.hpp"

namespace Nif
{
    void Extra::read(NIFStream *nif)
    {
        if (nif->getVersion() >= NIFStream::generateVersion(10,0,1,0))
            name = nif->getString();
        else if (nif->getVersion() <= NIFStream::generateVersion(4,2,2,0))
        {
            next.read(nif);
            recordSize = nif->getUInt();
        }
    }

    void Named::read(NIFStream *nif)
    {
        name = nif->getString();
        if (nif->getVersion() < NIFStream::generateVersion(10,0,1,0))
            extra.read(nif);
        else
            extralist.read(nif);
        controller.read(nif);
    }

    void Named::post(NIFFile *nif)
    {
        extra.post(nif);
        extralist.post(nif);
        controller.post(nif);
    }
}
