#include "idtypedelegate.hpp"

#include "../../model/world/universalid.hpp"

CSVWorld::IdTypeDelegate::IdTypeDelegate
    (const ValueList &values, const IconList &icons, QUndoStack& undoStack, QObject *parent)
    : DataDisplayDelegate (values, icons, undoStack,
                           "Display Format", "Referenceable ID Type Display",
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

CSVWorld::CommandDelegate *CSVWorld::IdTypeDelegateFactory::makeDelegate (QUndoStack& undoStack,
    QObject *parent) const
{
    return new IdTypeDelegate (mValues, mIcons, undoStack, parent);
}
