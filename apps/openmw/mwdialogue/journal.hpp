#ifndef GAME_MMDIALOG_JOURNAL_H
#define GAME_MWDIALOG_JOURNAL_H

namespace MWWorld
{
    struct Environment;
}

namespace MWDialogue
{
    class Journal
    {
            MWWorld::Environment& mEnvironment;

        public:

            Journal (MWWorld::Environment& environment);
    };
}

#endif
