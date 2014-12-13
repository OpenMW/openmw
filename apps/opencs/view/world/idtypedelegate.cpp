#include "idtypedelegate.hpp"

#include "../../model/world/universalid.hpp"

CSVWorld::IdTypeDelegate::IdTypeDelegate
    (const ValueList &values, const IconList &icons, CSMDoc::Document& document, QObject *parent)
    : DataDisplayDelegate (values, icons, document,
                           "records", "type-format",
                           parent)
{}

CSVWorld::IdTypeDelegateFactory::IdTypeDelegateFactory()
{
    for (int i=0; i<CSMWorld::UniversalId::NumberOfTypes; ++i)
    {
        CSMWorld::UniversalId id (static_cast<CSMWorld::UniversalId::Type> (i));

        DataDisplayDelegateFactory::add (id.getType(), QString::fromUtf8 (id.getTypeName().c_str()),
            QString::fromUtf8 (id.getIcon().c_str()));
    }
}

CSVWorld::CommandDelegate *CSVWorld::IdTypeDelegateFactory::makeDelegate (
    CSMDoc::Document& document, QObject *parent) const
{
    return new IdTypeDelegate (mValues, mIcons, document, parent);
}
