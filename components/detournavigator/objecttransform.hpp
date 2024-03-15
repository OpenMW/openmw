#ifndef OPENMW_COMPONENTS_DETOURNAVIGATOR_OBJECTTRANSFORM_H
#define OPENMW_COMPONENTS_DETOURNAVIGATOR_OBJECTTRANSFORM_H

#include <components/esm/position.hpp>

#include <tuple>

namespace DetourNavigator
{
    struct ObjectTransform
    {
        ESM::Position mPosition;
        float mScale;

        friend inline auto tie(const ObjectTransform& v) { return std::tie(v.mPosition, v.mScale); }

        friend inline bool operator<(const ObjectTransform& l, const ObjectTransform& r) { return tie(l) < tie(r); }
    };
}

#endif
