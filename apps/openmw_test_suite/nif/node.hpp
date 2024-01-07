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
        value.mController = NiTimeControllerPtr(nullptr);
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
        value.mData = NiGeometryDataPtr(nullptr);
        value.mSkin = NiSkinInstancePtr(nullptr);
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
        value.mPartitions = NiSkinPartitionPtr(nullptr);
    }

    inline void init(NiTimeController& value)
    {
        value.mNext = NiTimeControllerPtr(nullptr);
        value.mFlags = 0;
        value.mFrequency = 0;
        value.mPhase = 0;
        value.mTimeStart = 0;
        value.mTimeStop = 0;
        value.mTarget = NiObjectNETPtr(nullptr);
    }
}

#endif
