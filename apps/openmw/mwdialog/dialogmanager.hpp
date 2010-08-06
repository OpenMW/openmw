#ifndef GAME_MMDIALOG_DIALOGMANAGER_H
#define GAME_MWDIALOG_DIALOGMANAGER_H

#include "../mwworld/ptr.hpp"

namespace MWWorld
{
    class Environment;
}

namespace MWDialog
{
    class DialogManager
    {
            MWWorld::Environment& mEnvironment;

        public:

            DialogManager (MWWorld::Environment& environment);

            void startDialog (const MWWorld::Ptr& actor);

    };
}

#endif
