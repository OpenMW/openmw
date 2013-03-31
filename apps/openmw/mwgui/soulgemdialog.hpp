#ifndef OPENMW_MWGUI_SOULGEMDIALOG_H
#define OPENMW_MWGUI_SOULGEMDIALOG_H

#include "../mwworld/ptr.hpp"

namespace MWGui
{

    class MessageBoxManager;

    class SoulgemDialog
    {
    public:
        SoulgemDialog (MessageBoxManager* manager)
            : mManager(manager) {}

        void show (const MWWorld::Ptr& soulgem);

        void onButtonPressed(int button);

    private:
        MessageBoxManager* mManager;
        MWWorld::Ptr mSoulgem;
    };

}

#endif
