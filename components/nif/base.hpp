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
/** A record that can have extra data. The extra data objects
    themselves descend from the Extra class, and all the extra data
    connected to an object form a linked list
*/
class Extra : public Record
{
public:
    ExtraPtr extra;

    void read(NIFStream *nif) { extra.read(nif); }
    void post(NIFFile *nif) { extra.post(nif); }
};

class Controller : public Record
{
public:
    ControllerPtr next;
    int flags;
    float frequency, phase;
    float timeStart, timeStop;
    ControlledPtr target;

    void read(NIFStream *nif);
    void post(NIFFile *nif);
};

/// Anything that has a controller
class Controlled : public Extra
{
public:
    ControllerPtr controller;

    void read(NIFStream *nif)
    {
        Extra::read(nif);
        controller.read(nif);
    }

    void post(NIFFile *nif)
    {
        Extra::post(nif);
        controller.post(nif);
    }
};

/// Has name, extra-data and controller
class Named : public Controlled
{
public:
    std::string name;

    void read(NIFStream *nif)
    {
        name = nif->getString();
        Controlled::read(nif);
    }
};
typedef Named NiSequenceStreamHelper;

} // Namespace
#endif
