#ifndef OPENMW_ESM_OBJECTSTATE_H
#define OPENMW_ESM_OBJECTSTATE_H

#include <string>
#include <vector>

#include "cellref.hpp"
#include "locals.hpp"

namespace ESM
{
    class ESMReader;
    class ESMWriter;

    // format 0, saved games only

    ///< \brief Save state for objects, that do not use custom data
    struct ObjectState
    {
        CellRef mRef;

        unsigned char mHasLocals;
        Locals mLocals;
        unsigned char mEnabled;
        int mCount;
        ESM::Position mPosition;
        float mLocalRotation[3];

        // Is there any class-specific state following the ObjectState
        bool mHasCustomState;

        unsigned int mVersion;

        ObjectState() : mHasCustomState(true), mVersion(0)
        {}

        /// @note Does not load the CellRef ID, it should already be loaded before calling this method
        virtual void load (ESMReader &esm);

        virtual void save (ESMWriter &esm, bool inInventory = false) const;

        /// Initialize to default state
        void blank();

        virtual ~ObjectState();
    };
}

#endif
