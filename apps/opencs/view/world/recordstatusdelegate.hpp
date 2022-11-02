#ifndef RECORDSTATUSDELEGATE_H
#define RECORDSTATUSDELEGATE_H

#include "datadisplaydelegate.hpp"

class QObject;

namespace CSMDoc
{
    class Document;
}

namespace CSMWorld
{
    class CommandDispatcher;
}

namespace CSVWorld
{
    class CommandDelegate;

    class RecordStatusDelegate : public DataDisplayDelegate
    {
    public:
        RecordStatusDelegate(const ValueList& values, const IconList& icons, CSMWorld::CommandDispatcher* dispatcher,
            CSMDoc::Document& document, QObject* parent = nullptr);
    };

    class RecordStatusDelegateFactory : public DataDisplayDelegateFactory
    {
    public:
        RecordStatusDelegateFactory();

        CommandDelegate* makeDelegate(
            CSMWorld::CommandDispatcher* dispatcher, CSMDoc::Document& document, QObject* parent) const override;
        ///< The ownership of the returned CommandDelegate is transferred to the caller.
    };
}
#endif // RECORDSTATUSDELEGATE_HPP
