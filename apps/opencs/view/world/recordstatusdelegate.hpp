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
                                      CSMDoc::Document& document, QObject *parent = 0);
    };

    class RecordStatusDelegateFactory : public DataDisplayDelegateFactory
    {
        public:

            RecordStatusDelegateFactory();

            virtual CommandDelegate *makeDelegate (CSMDoc::Document& document, QObject *parent) const;
            ///< The ownership of the returned CommandDelegate is transferred to the caller.

    };
}
#endif // RECORDSTATUSDELEGATE_HPP

