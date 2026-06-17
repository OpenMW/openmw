#include "infocreator.hpp"

#include <algorithm>
#include <charconv>
#include <memory>

#include <QLabel>
#include <QRandomGenerator>
#include <QString>

#include <apps/opencs/model/world/columnbase.hpp>
#include <apps/opencs/model/world/idcollection.hpp>
#include <apps/opencs/view/world/genericcreator.hpp>

#include <components/misc/strings/lower.hpp>

#include "../../model/doc/document.hpp"

#include "../../model/world/columns.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/world/data.hpp"
#include "../../model/world/idcompletionmanager.hpp"
#include "../../model/world/idtable.hpp"

#include "../widget/droplineedit.hpp"

class QUndoStack;

std::string CSVWorld::InfoCreator::getId() const
{
    std::string id = mTopic->text().toStdString();
    size_t length = id.size();
    // We want generated ids to be at most 31 + \0 characters
    id.resize(length + 32);
    id[length] = '#';
    // Combine a random 32bit number with a random 64bit number for a max 30 character string
    quint32 gen32 = QRandomGenerator::global()->generate();
    char* start = id.data() + length + 1;
    char* end = start + 10; // 2^32 is a 10 digit number
    auto result = std::to_chars(start, end, gen32);
    quint64 gen64 = QRandomGenerator::global()->generate64();
    if (gen64)
    {
        // 0-pad the first number so 10 + 11 isn't the same as 101 + 1
        std::fill(result.ptr, end, '0');
        start = end;
        end = start + 20; // 2^64 is a 20 digit number
        result = std::to_chars(start, end, gen64);
    }
    id.resize(result.ptr - id.data());
    return id;
}

void CSVWorld::InfoCreator::configureCreateCommand(CSMWorld::CreateCommand& command) const
{
    CSMWorld::IdTable& table = dynamic_cast<CSMWorld::IdTable&>(*getData().getTableModel(getCollectionId()));

    CSMWorld::CloneCommand* cloneCommand = dynamic_cast<CSMWorld::CloneCommand*>(&command);
    if (getCollectionId() == CSMWorld::UniversalId::Type_TopicInfos)
    {
        if (!cloneCommand)
        {
            command.addValue(table.findColumnIndex(CSMWorld::Columns::ColumnId_Topic), mTopic->text());
            command.addValue(table.findColumnIndex(CSMWorld::Columns::ColumnId_Rank), -1);
            command.addValue(table.findColumnIndex(CSMWorld::Columns::ColumnId_Gender), -1);
            command.addValue(table.findColumnIndex(CSMWorld::Columns::ColumnId_PcRank), -1);
        }
        else
        {
            cloneCommand->setOverrideValue(table.findColumnIndex(CSMWorld::Columns::ColumnId_Topic), mTopic->text());
        }
    }
    else
    {
        if (!cloneCommand)
        {
            command.addValue(table.findColumnIndex(CSMWorld::Columns::ColumnId_Journal), mTopic->text());
        }
        else
            cloneCommand->setOverrideValue(table.findColumnIndex(CSMWorld::Columns::ColumnId_Journal), mTopic->text());
    }
}

CSVWorld::InfoCreator::InfoCreator(CSMWorld::Data& worldData, QUndoStack& undoStack, const CSMWorld::UniversalId& id,
    CSMWorld::IdCompletionManager& completionManager)
    : GenericCreator(worldData, undoStack, id)
{
    // Determine if we're dealing with topics or journals.
    CSMWorld::ColumnBase::Display displayType = CSMWorld::ColumnBase::Display_Topic;
    QString labelText = "Topic";
    if (getCollectionId().getType() == CSMWorld::UniversalId::Type_JournalInfos)
    {
        displayType = CSMWorld::ColumnBase::Display_Journal;
        labelText = "Journal";
    }

    QLabel* label = new QLabel(labelText, this);
    insertBeforeButtons(label, false);

    // Add topic/journal ID input with auto-completion.
    // Only existing topic/journal IDs are accepted so no ID validation is performed.
    mTopic = new CSVWidget::DropLineEdit(displayType, this);
    mTopic->setCompleter(completionManager.getCompleter(displayType).get());
    insertBeforeButtons(mTopic, true);

    setManualEditing(false);

    connect(mTopic, &CSVWidget::DropLineEdit::textChanged, this, &InfoCreator::topicChanged);
    connect(mTopic, &CSVWidget::DropLineEdit::returnPressed, this, &InfoCreator::inputReturnPressed);
}

void CSVWorld::InfoCreator::cloneMode(const std::string& originId, const CSMWorld::UniversalId::Type type)
{
    CSMWorld::IdTable& infoTable = dynamic_cast<CSMWorld::IdTable&>(*getData().getTableModel(getCollectionId()));

    int topicColumn = infoTable.findColumnIndex(getCollectionId().getType() == CSMWorld::UniversalId::Type_TopicInfos
            ? CSMWorld::Columns::ColumnId_Topic
            : CSMWorld::Columns::ColumnId_Journal);

    mTopic->setText(infoTable.data(infoTable.getModelIndex(originId, topicColumn)).toString());

    GenericCreator::cloneMode(originId, type);
}

void CSVWorld::InfoCreator::reset()
{
    mTopic->setText("");
    GenericCreator::reset();
}

void CSVWorld::InfoCreator::setText(const std::string& text)
{
    QString qText = QString::fromStdString(text);
    mTopic->setText(qText);
}

std::string CSVWorld::InfoCreator::getErrors() const
{
    // We ignore errors from GenericCreator here, because they can never happen in an InfoCreator.
    std::string errors;

    const ESM::RefId topic = ESM::RefId::stringRefId(mTopic->text().toStdString());

    if ((getCollectionId().getType() == CSMWorld::UniversalId::Type_TopicInfos ? getData().getTopics()
                                                                               : getData().getJournals())
            .searchId(topic)
        == -1)
    {
        errors += "Invalid Topic ID";
    }

    return errors;
}

void CSVWorld::InfoCreator::focus()
{
    mTopic->setFocus();
}

void CSVWorld::InfoCreator::callReturnPressed()
{
    emit inputReturnPressed();
}

void CSVWorld::InfoCreator::topicChanged()
{
    update();
}

CSVWorld::Creator* CSVWorld::InfoCreatorFactory::makeCreator(
    CSMDoc::Document& document, const CSMWorld::UniversalId& id) const
{
    return new InfoCreator(document.getData(), document.getUndoStack(), id, document.getIdCompletionManager());
}
