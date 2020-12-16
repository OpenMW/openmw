///This file holds the main classes of NIF Records used by everything else.
#ifndef OPENMW_COMPONENTS_NIF_BASE_HPP
#define OPENMW_COMPONENTS_NIF_BASE_HPP

#include "record.hpp"
#include "niffile.hpp"
#include "recordptr.hpp"
#include "nifstream.hpp"
#include "nifkey.hpp"

namespace Nif
{
// An extra data record. All the extra data connected to an object form a linked list.
struct Extra : public Record
{
    std::string name;
    ExtraPtr next; // Next extra data record in the list

    void read(NIFStream *nif) override
    {
        if (nif->getVersion() >= NIFStream::generateVersion(10,0,1,0))
            name = nif->getString();
        else if (nif->getVersion() <= NIFStream::generateVersion(4,2,2,0))
        {
            next.read(nif);
            nif->getUInt(); // Size of the record
        }
    }

    void post(NIFFile *nif) override { next.post(nif); }
};

struct Controller : public Record
{
    ControllerPtr next;
    int flags;
    float frequency, phase;
    float timeStart, timeStop;
    NamedPtr target;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};

/// Has name, extra-data and controller
struct Named : public Record
{
    std::string name;
    ExtraPtr extra;
    ExtraList extralist;
    ControllerPtr controller;

    void read(NIFStream *nif) override
    {
        name = nif->getString();
        if (nif->getVersion() < NIFStream::generateVersion(10,0,1,0))
            extra.read(nif);
        else
            extralist.read(nif);
        controller.read(nif);
    }

    void post(NIFFile *nif) override
    {
        extra.post(nif);
        extralist.post(nif);
        controller.post(nif);
    }
};
using NiSequenceStreamHelper = Named;

} // Namespace
#endif
