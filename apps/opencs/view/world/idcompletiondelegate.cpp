#include "idcompletiondelegate.hpp"

#include "../../model/world/idcompletionmanager.hpp"
#include "../../model/world/infoselectwrapper.hpp"

#include <apps/opencs/model/doc/document.hpp>
#include <apps/opencs/view/world/util.hpp>

#include <memory>

#include "../widget/droplineedit.hpp"

namespace CSMWorld
{
    class CommandDispatcher;
}

class QObject;
class QWidget;

CSVWorld::IdCompletionDelegate::IdCompletionDelegate(
    CSMWorld::CommandDispatcher* dispatcher, CSMDoc::Document& document, QObject* parent)
    : CommandDelegate(dispatcher, document, parent)
{
}

QWidget* CSVWorld::IdCompletionDelegate::createEditor(
    QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return createEditor(parent, option, index, getDisplayTypeFromIndex(index));
}

QWidget* CSVWorld::IdCompletionDelegate::createEditor(QWidget* parent, const QStyleOptionViewItem& option,
    const QModelIndex& index, CSMWorld::ColumnBase::Display display) const
{
    if (!index.data(Qt::EditRole).isValid() && !index.data(Qt::DisplayRole).isValid())
    {
        return nullptr;
    }

    // The completer for InfoCondVar needs to return a completer based on the first column
    if (display == CSMWorld::ColumnBase::Display_InfoCondVar)
    {
        QModelIndex sibling = index.sibling(index.row(), 0);
        int conditionFunction = sibling.model()->data(sibling, Qt::EditRole).toInt();

        switch (conditionFunction)
        {
            case ESM::DialogueCondition::Function_Global:
            {
                return createEditor(parent, option, index, CSMWorld::ColumnBase::Display_GlobalVariable);
            }
            case ESM::DialogueCondition::Function_Journal:
            {
                return createEditor(parent, option, index, CSMWorld::ColumnBase::Display_Journal);
            }
            case ESM::DialogueCondition::Function_Item:
            {
                return createEditor(parent, option, index, CSMWorld::ColumnBase::Display_Referenceable);
            }
            case ESM::DialogueCondition::Function_Dead:
            case ESM::DialogueCondition::Function_NotId:
            {
                return createEditor(parent, option, index, CSMWorld::ColumnBase::Display_Referenceable);
            }
            case ESM::DialogueCondition::Function_NotFaction:
            {
                return createEditor(parent, option, index, CSMWorld::ColumnBase::Display_Faction);
            }
            case ESM::DialogueCondition::Function_NotClass:
            {
                return createEditor(parent, option, index, CSMWorld::ColumnBase::Display_Class);
            }
            case ESM::DialogueCondition::Function_NotRace:
            {
                return createEditor(parent, option, index, CSMWorld::ColumnBase::Display_Race);
            }
            case ESM::DialogueCondition::Function_NotCell:
            {
                return createEditor(parent, option, index, CSMWorld::ColumnBase::Display_Cell);
            }
            case ESM::DialogueCondition::Function_Local:
            case ESM::DialogueCondition::Function_NotLocal:
            {
                return new CSVWidget::DropLineEdit(display, parent);
            }
            default:
                return nullptr; // The rest of them can't be edited anyway
        }
    }

    CSMWorld::IdCompletionManager& completionManager = getDocument().getIdCompletionManager();
    CSVWidget::DropLineEdit* editor = new CSVWidget::DropLineEdit(display, parent);
    editor->setCompleter(completionManager.getCompleter(display).get());

    // The savegame format limits the player faction string to 32 characters.
    // The region sound name is limited to 32 characters. (ESM::Region::SoundRef::mSound)
    // The script name is limited to 32 characters. (ESM::Script::SCHD::mName)
    // The cell name is limited to 64 characters. (ESM::Header::GMDT::mCurrentCell)
    if (display == CSMWorld::ColumnBase::Display_Faction || display == CSMWorld::ColumnBase::Display_Sound
        || display == CSMWorld::ColumnBase::Display_Script || display == CSMWorld::ColumnBase::Display_Referenceable)
    {
        editor->setMaxLength(32);
    }
    else if (display == CSMWorld::ColumnBase::Display_Cell)
    {
        editor->setMaxLength(64);
    }

    return editor;
}

CSVWorld::CommandDelegate* CSVWorld::IdCompletionDelegateFactory::makeDelegate(
    CSMWorld::CommandDispatcher* dispatcher, CSMDoc::Document& document, QObject* parent) const
{
    return new IdCompletionDelegate(dispatcher, document, parent);
}
