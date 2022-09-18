#ifndef OPENMW_TEST_SUITE_NIF_NODE_H
#define OPENMW_TEST_SUITE_NIF_NODE_H

#include <components/nif/data.hpp>
#include <components/nif/node.hpp>

namespace Nif::Testing
{
    inline void init(Transformation& value)
    {
        value = Transformation::getIdentity();
    }

    inline void init(Extra& value)
    {
        value.next = ExtraPtr(nullptr);
    }

    inline void init(Named& value)
    {
        value.extra = ExtraPtr(nullptr);
        value.extralist = ExtraList();
        value.controller = ControllerPtr(nullptr);
    }

    inline void init(Node& value)
    {
        init(static_cast<Named&>(value));
        value.flags = 0;
        init(value.trafo);
        value.hasBounds = false;
        value.isBone = false;
    }

    inline void init(NiGeometry& value)
    {
        init(static_cast<Node&>(value));
        value.data = NiGeometryDataPtr(nullptr);
        value.skin = NiSkinInstancePtr(nullptr);
    }

    inline void init(NiTriShape& value)
    {
        init(static_cast<NiGeometry&>(value));
        value.recType = RC_NiTriShape;
    }

    inline void init(NiTriStrips& value)
    {
        init(static_cast<NiGeometry&>(value));
        value.recType = RC_NiTriStrips;
    }

    inline void init(NiSkinInstance& value)
    {
        value.data = NiSkinDataPtr(nullptr);
        value.root = NodePtr(nullptr);
    }

    inline void init(Controller& value)
    {
        value.next = ControllerPtr(nullptr);
        value.flags = 0;
        value.frequency = 0;
        value.phase = 0;
        value.timeStart = 0;
        value.timeStop = 0;
        value.target = NamedPtr(nullptr);
    }
}

#endif
