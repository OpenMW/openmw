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
class Extra : public Record
{
public:
    ExtraPtr next; // Next extra data record in the list

    void read(NIFStream *nif)
    {
        next.read(nif);
        nif->getUInt(); // Size of the record
    }

    void post(NIFFile *nif) { next.post(nif); }
};

class Controller : public Record
{
public:
    ControllerPtr next;
    int flags;
    float frequency, phase;
    float timeStart, timeStop;
    NamedPtr target;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

/// Has name, extra-data and controller
class Named : public Record
{
public:
    std::string name;
    ExtraPtr extra;
    ControllerPtr controller;

    void read(NIFStream *nif)
    {
        name = nif->getString();
        extra.read(nif);
        controller.read(nif);
    }

    void post(NIFFile *nif)
    {
        extra.post(nif);
        controller.post(nif);
    }
};
using NiSequenceStreamHelper = Named;

} // Namespace
#endif
