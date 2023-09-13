#ifndef OPENMW_TEST_SUITE_NIF_NODE_H
#define OPENMW_TEST_SUITE_NIF_NODE_H

#include <components/nif/data.hpp>
#include <components/nif/node.hpp>

namespace Nif::Testing
{
    inline void init(NiTransform& value)
    {
        value = NiTransform::getIdentity();
    }

    inline void init(Extra& value)
    {
        value.mNext = ExtraPtr(nullptr);
    }

    inline void init(NiObjectNET& value)
    {
        value.mExtra = ExtraPtr(nullptr);
        value.mExtraList = ExtraList();
        value.mController = ControllerPtr(nullptr);
    }

    inline void init(NiAVObject& value)
    {
        init(static_cast<NiObjectNET&>(value));
        value.mFlags = 0;
        init(value.mTransform);
    }

    inline void init(NiGeometry& value)
    {
        init(static_cast<NiAVObject&>(value));
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
        value.mData = NiSkinDataPtr(nullptr);
        value.mRoot = NiAVObjectPtr(nullptr);
    }

    inline void init(Controller& value)
    {
        value.next = ControllerPtr(nullptr);
        value.flags = 0;
        value.frequency = 0;
        value.phase = 0;
        value.timeStart = 0;
        value.timeStop = 0;
        value.mTarget = NiObjectNETPtr(nullptr);
    }
}

#endif
