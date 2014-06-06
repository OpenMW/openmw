#ifndef CSM_WOLRD_COMMANDDISPATCHER_H
#define CSM_WOLRD_COMMANDDISPATCHER_H

#include <vector>

#include <QObject>

#include "universalid.hpp"

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class CommandDispatcher : public QObject
    {
            Q_OBJECT

            bool mLocked;
            CSMDoc::Document& mDocument;
            UniversalId mId;
            std::vector<int> mSelection;

            std::vector<int> getDeletableRecords() const;

            std::vector<int> getRevertableRecords() const;

        public:

            CommandDispatcher (CSMDoc::Document& document, const CSMWorld::UniversalId& id,
                QObject *parent = 0);
            ///< \param id ID of the table the commands should operate on primarily.

            void setEditLock (bool locked);

            void setSelection (const std::vector<int>& selection);

            bool canDelete() const;

            bool canRevert() const;

        public slots:

            void executeDelete();

            void executeRevert();

    };
}

#endif
