#ifndef MWGUI_REFERENCEINTERFACE_H
#define MWGUI_REFERENCEINTERFACE_H

#include "../mwworld/ptr.hpp"

namespace MWGui
{
    /// \brief this class is intended for GUI interfaces that access an MW-Reference
    /// for example dialogue window accesses an NPC, or Container window accesses a Container
    /// these classes have to be automatically closed if the reference becomes unavailable
    /// make sure that checkReferenceAvailable() is called every frame and that onReferenceUnavailable() has been overridden
    class ReferenceInterface
    {
    public:
        ReferenceInterface();
        virtual ~ReferenceInterface();

        void checkReferenceAvailable(); ///< closes the window, if the MW-reference has become unavailable

        virtual void resetReference() { mPtr = MWWorld::Ptr(); }

    protected:
        virtual void onReferenceUnavailable() = 0; ///< called when reference has become unavailable

        MWWorld::Ptr mPtr;
    };
}

#endif
