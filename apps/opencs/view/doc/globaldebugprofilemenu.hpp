#ifndef CSV_DOC_GLOBALDEBUGPROFILEMENU_H
#define CSV_DOC_GLOBALDEBUGPROFILEMENU_H

#include <QMenu>

class QModelIndex;
class QActionGroup;

namespace CSMWorld
{
    class IdTable;
}

namespace CSVDoc
{
    class GlobalDebugProfileMenu : public QMenu
    {
            Q_OBJECT

            CSMWorld::IdTable *mDebugProfiles;
            QActionGroup *mActions;

        private:

            void rebuild();

        public:

            GlobalDebugProfileMenu (CSMWorld::IdTable *debugProfiles, QWidget *parent = nullptr);

            void updateActions (bool running);

        private slots:

            void profileAboutToBeRemoved (const QModelIndex& parent, int start, int end);

            void profileInserted (const QModelIndex& parent, int start, int end);

            void profileChanged (const QModelIndex& topLeft, const QModelIndex& bottomRight);

            void actionTriggered (QAction *action);

        signals:

            void triggered (const std::string& profile);
    };
}

#endif
