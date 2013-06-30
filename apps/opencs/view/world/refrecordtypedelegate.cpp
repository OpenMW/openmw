#include "refrecordtypedelegate.hpp"
#include "../../model/world/universalid.hpp"

#include <QDebug>

CSVWorld::RefRecordTypeDelegate::RefRecordTypeDelegate
    (const std::vector<std::pair<int, QString> > &values, QUndoStack& undoStack, QObject *parent)
        : EnumDelegate (values, undoStack, parent)
{}

CSVWorld::RefRecordTypeDelegateFactory::RefRecordTypeDelegateFactory()
{
    unsigned int argSize = CSMWorld::UniversalId::getIdArgSize();

    for (unsigned int i = 0; i < argSize; i++)
    {
        std::pair<int, const char *> idPair = CSMWorld::UniversalId::getIdArgPair(i);

        mValues.push_back (std::pair<int, QString>(idPair.first, QString::fromUtf8(idPair.second)));

        qDebug() << "index: " << mValues.at(i).first << "; value: " << mValues.at(i).second;
    }
}

CSVWorld::CommandDelegate *CSVWorld::RefRecordTypeDelegateFactory::makeDelegate (QUndoStack& undoStack,
    QObject *parent) const
{
    return new RefRecordTypeDelegate (mValues, undoStack, parent);
}
