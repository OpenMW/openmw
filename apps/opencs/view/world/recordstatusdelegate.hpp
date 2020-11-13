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

        RecordStatusDelegate (const ValueList& values, const IconList& icons,
            CSMWorld::CommandDispatcher *dispatcher, CSMDoc::Document& document,
            QObject *parent = nullptr);
    };

    class RecordStatusDelegateFactory : public DataDisplayDelegateFactory
    {
        public:

            RecordStatusDelegateFactory();

            CommandDelegate *makeDelegate (CSMWorld::CommandDispatcher *dispatcher, CSMDoc::Document& document, QObject *parent) const override;
            ///< The ownership of the returned CommandDelegate is transferred to the caller.

    };
}
#endif // RECORDSTATUSDELEGATE_HPP

