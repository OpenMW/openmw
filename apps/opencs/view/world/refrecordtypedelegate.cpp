#include "refrecordtypedelegate.hpp"

CSVWorld::RefRecordTypeDelegate::RefRecordTypeDelegate
    (const std::vector<std::pair<int, QString> > &values, QUndoStack& undoStack, QObject *parent)
        : EnumDelegate (values, undoStack, parent)
{}

CSVWorld::RefRecordTypeDelegateFactory::RefRecordTypeDelegateFactory()
{

}

CSVWorld::CommandDelegate *CSVWorld::RefRecordTypeDelegateFactory::makeDelegate (QUndoStack& undoStack,
    QObject *parent) const
{
    return new RefRecordTypeDelegate (mValues, undoStack, parent);
}
