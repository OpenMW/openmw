#include "referenceinterface.hpp"

namespace MWGui
{
    ReferenceInterface::ReferenceInterface() = default;

    ReferenceInterface::~ReferenceInterface() = default;

    void ReferenceInterface::checkReferenceAvailable()
    {
        // check if count of the reference has become 0
        if (!mPtr.isEmpty() && mPtr.getCellRef().getCount() == 0)
        {
            mPtr = MWWorld::Ptr();
            onReferenceUnavailable();
        }
    }
}
