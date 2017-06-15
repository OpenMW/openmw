#ifndef CSM_WORLD_IDCOMPLETIONMANAGER_HPP
#define CSM_WORLD_IDCOMPLETIONMANAGER_HPP

#include <vector>
#include <map>
#include <memory>

#include "columnbase.hpp"
#include "universalid.hpp"

class QCompleter;

namespace CSMWorld
{
    class Data;

    /// \brief Creates and stores all ID completers
    class IdCompletionManager
    {
            static const std::map<ColumnBase::Display, UniversalId::Type> sCompleterModelTypes;

            std::map<ColumnBase::Display, std::shared_ptr<QCompleter> > mCompleters;

            // Don't allow copying
            IdCompletionManager(const IdCompletionManager &);
            IdCompletionManager &operator = (const IdCompletionManager &);

            void generateCompleters(Data &data);

        public:
            static std::vector<ColumnBase::Display> getDisplayTypes();

            IdCompletionManager(Data &data);

            bool hasCompleterFor(ColumnBase::Display display) const;
            std::shared_ptr<QCompleter> getCompleter(ColumnBase::Display display);
    };
}

#endif
