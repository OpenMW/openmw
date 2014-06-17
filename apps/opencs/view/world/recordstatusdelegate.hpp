#ifndef RECORDSTATUSDELEGATE_H
#define RECORDSTATUSDELEGATE_H

#include "util.hpp"
#include <QTextOption>
#include <QFont>

#include "datadisplaydelegate.hpp"
#include "../../model/world/record.hpp"

class QIcon;
class QFont;

namespace CSVWorld
{
    class RecordStatusDelegate : public DataDisplayDelegate
    {
    public:

        explicit RecordStatusDelegate(const ValueList& values,
                                      const IconList& icons,
                                      QUndoStack& undoStack, QObject *parent = 0);
    };

    class RecordStatusDelegateFactory : public DataDisplayDelegateFactory
    {
        public:

            RecordStatusDelegateFactory();

            virtual CommandDelegate *makeDelegate (QUndoStack& undoStack, QObject *parent) const;
            ///< The ownership of the returned CommandDelegate is transferred to the caller.

    };
}
#endif // RECORDSTATUSDELEGATE_HPP

