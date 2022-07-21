///This file holds the main classes of NIF Records used by everything else.
#ifndef OPENMW_COMPONENTS_NIF_BASE_HPP
#define OPENMW_COMPONENTS_NIF_BASE_HPP

#include "recordptr.hpp"

namespace Nif
{
struct File;
struct Record;
struct Stream;

// An extra data record. All the extra data connected to an object form a linked list.
struct Extra : public Record
{
    std::string name;
    ExtraPtr next; // Next extra data record in the list
    unsigned int recordSize{0u};

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override { next.post(nif); }
};

struct Controller : public Record
{
    enum Flags {
        Flag_Active = 0x8
    };

    enum ExtrapolationMode
    {
        Cycle = 0,
        Reverse = 2,
        Constant = 4,
        Mask = 6
    };

    ControllerPtr next;
    int flags;
    float frequency, phase;
    float timeStart, timeStop;
    NamedPtr target;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;

    bool isActive() const { return flags & Flag_Active; }
    ExtrapolationMode extrapolationMode() const { return static_cast<ExtrapolationMode>(flags & Mask); }
};

/// Has name, extra-data and controller
struct Named : public Record
{
    std::string name;
    ExtraPtr extra;
    ExtraList extralist;
    ControllerPtr controller;

    void read(NIFStream *nif) override;
    void post(NIFFile *nif) override;
};
using NiSequenceStreamHelper = Named;

} // Namespace
#endif
