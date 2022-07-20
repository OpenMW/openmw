#ifndef OPENMW_COMPONENTS_BULLETHELPERS_HEIGHTFIELD_H
#define OPENMW_COMPONENTS_BULLETHELPERS_HEIGHTFIELD_H

#include <LinearMath/btVector3.h>

namespace BulletHelpers
{
    inline btVector3 getHeightfieldShift(int x, int y, int size, float minHeight, float maxHeight)
    {
        return btVector3((x + 0.5f) * size, (y + 0.5f) * size, (maxHeight + minHeight) * 0.5f);
    }
}

#endif
