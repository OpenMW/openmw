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
            std::vector<std::string> mSelection;
            std::vector<UniversalId> mExtendedTypes;

            std::vector<std::string> getDeletableRecords() const;

            std::vector<std::string> getRevertableRecords() const;

        public:

            CommandDispatcher (CSMDoc::Document& document, const CSMWorld::UniversalId& id,
                QObject *parent = 0);
            ///< \param id ID of the table the commands should operate on primarily.

            void setEditLock (bool locked);

            void setSelection (const std::vector<std::string>& selection);

            void setExtendedTypes (const std::vector<UniversalId>& types);
            ///< Set record lists selected by the user for extended operations.

            bool canDelete() const;

            bool canRevert() const;

            /// Return IDs of the record collection that can also be affected when
            /// operating on the record collection this dispatcher is used for.
            ///
            /// \note The returned collection contains the ID of the record collection this
            /// dispatcher is used for. However if that record collection does not support
            /// the extended mode, the returned vector will be empty instead.
            std::vector<UniversalId> getExtendedTypes() const;

        public slots:

            void executeDelete();

            void executeRevert();

            void executeExtendedDelete();

            void executeExtendedRevert();

    };
}

#endif
