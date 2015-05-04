
#include "infocreator.hpp"

#include <algorithm>

#include <QLabel>
#include <QLineEdit>
#include <QUuid>

#include <components/misc/stringops.hpp>

#include "../../model/world/data.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/idtable.hpp"

std::string CSVWorld::InfoCreator::getId() const
{
    std::string id = Misc::StringUtils::lowerCase (mTopic->text().toUtf8().constData());

    std::string unique = QUuid::createUuid().toByteArray().data();

    unique.erase (std::remove (unique.begin(), unique.end(), '-'), unique.end());

    unique = unique.substr (1, unique.size()-2);

    return id + '#' + unique;
}

void CSVWorld::InfoCreator::configureCreateCommand (CSMWorld::CreateCommand& command) const
{
    int index =
        dynamic_cast<CSMWorld::IdTable&> (*getData().getTableModel (getCollectionId())).
        findColumnIndex (
        getCollectionId().getType()==CSMWorld::UniversalId::Type_TopicInfos ?
        CSMWorld::Columns::ColumnId_Topic : CSMWorld::Columns::ColumnId_Journal);

    command.addValue (index, mTopic->text());
}

CSVWorld::InfoCreator::InfoCreator (CSMWorld::Data& data, QUndoStack& undoStack,
    const CSMWorld::UniversalId& id)
: GenericCreator (data, undoStack, id)
{
    QLabel *label = new QLabel ("Topic", this);
    insertBeforeButtons (label, false);

    mTopic = new QLineEdit (this);
    insertBeforeButtons (mTopic, true);

    setManualEditing (false);

    connect (mTopic, SIGNAL (textChanged (const QString&)), this, SLOT (topicChanged()));
}

void CSVWorld::InfoCreator::cloneMode (const std::string& originId,
    const CSMWorld::UniversalId::Type type)
{
    CSMWorld::IdTable& infoTable =
        dynamic_cast<CSMWorld::IdTable&> (*getData().getTableModel (getCollectionId()));

    int topicColumn = infoTable.findColumnIndex (
        getCollectionId().getType()==CSMWorld::UniversalId::Type_TopicInfos ?
        CSMWorld::Columns::ColumnId_Topic : CSMWorld::Columns::ColumnId_Journal);

    mTopic->setText (
        infoTable.data (infoTable.getModelIndex (originId, topicColumn)).toString());

    GenericCreator::cloneMode (originId, type);
}

void CSVWorld::InfoCreator::reset()
{
    mTopic->setText ("");
    GenericCreator::reset();
}

std::string CSVWorld::InfoCreator::getErrors() const
{
    // We ignore errors from GenericCreator here, because they can never happen in an InfoCreator.
    std::string errors;

    std::string topic = mTopic->text().toUtf8().constData();

    if ((getCollectionId().getType()==CSMWorld::UniversalId::Type_TopicInfos ?
        getData().getTopics() : getData().getJournals()).searchId (topic)==-1)
    {
        errors += "Invalid Topic ID";
    }

    return errors;
}

void CSVWorld::InfoCreator::focus()
{
    mTopic->setFocus();
}

void CSVWorld::InfoCreator::topicChanged()
{
    update();
}
