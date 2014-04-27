#ifndef IDTYPEDELEGATE_HPP
#define IDTYPEDELEGATE_HPP

#include "enumdelegate.hpp"
#include "util.hpp"
#include "../../model/world/universalid.hpp"
#include "datadisplaydelegate.hpp"

namespace CSVWorld
{
    class IdTypeDelegate : public DataDisplayDelegate
    {
        public:
            IdTypeDelegate (const ValueList &mValues, const IconList &icons, QUndoStack& undoStack, QObject *parent);
    };

    class IdTypeDelegateFactory : public DataDisplayDelegateFactory
    {
        public:

            IdTypeDelegateFactory();

            virtual CommandDelegate *makeDelegate (QUndoStack& undoStack, QObject *parent) const;
            ///< The ownership of the returned CommandDelegate is transferred to the caller.
    };
}

#endif // REFIDTYPEDELEGATE_HPP
