#ifndef CSM_WORLD_IDCOMPLETIONMANAGER_HPP
#define CSM_WORLD_IDCOMPLETIONMANAGER_HPP

#include <map>

#include <boost/shared_ptr.hpp>

#include "columns.hpp"
#include "universalid.hpp"

class QCompleter;

namespace CSMWorld
{
    class Data;

    /// \brief Creates and stores all ID completers
    class IdCompletionManager
    {
            static const std::map<CSMWorld::Columns::ColumnId, CSMWorld::UniversalId::Type> sCompleterModelTypes;

            std::map<CSMWorld::Columns::ColumnId, boost::shared_ptr<QCompleter> > mCompleters;

            // Don't allow copying
            IdCompletionManager(const IdCompletionManager &);
            IdCompletionManager &operator = (const IdCompletionManager &);

            void generateCompleters(CSMWorld::Data &data);

        public:
            IdCompletionManager(CSMWorld::Data &data);

            bool hasCompleterFor(CSMWorld::Columns::ColumnId id) const;
            boost::shared_ptr<QCompleter> getCompleter(CSMWorld::Columns::ColumnId id);
    };
}

#endif
