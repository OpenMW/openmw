#include "idcompletiondelegate.hpp"

#include "../../model/world/idcompletionmanager.hpp"
#include "../../model/world/infoselectwrapper.hpp"

#include "../widget/droplineedit.hpp"

CSVWorld::IdCompletionDelegate::IdCompletionDelegate(CSMWorld::CommandDispatcher *dispatcher,
                                                     CSMDoc::Document& document,
                                                     QObject *parent)
    : CommandDelegate(dispatcher, document, parent)
{}

QWidget *CSVWorld::IdCompletionDelegate::createEditor(QWidget *parent,
                                                      const QStyleOptionViewItem &option,
                                                      const QModelIndex &index) const
{
    return createEditor(parent, option, index, getDisplayTypeFromIndex(index));
}

QWidget *CSVWorld::IdCompletionDelegate::createEditor(QWidget *parent,
                                                      const QStyleOptionViewItem &option,
                                                      const QModelIndex &index,
                                                      CSMWorld::ColumnBase::Display display) const
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
            case CSMWorld::ConstInfoSelectWrapper::Function_Global:
            {
                return createEditor (parent, option, index, CSMWorld::ColumnBase::Display_GlobalVariable);
            }
            case CSMWorld::ConstInfoSelectWrapper::Function_Journal:
            {
                return createEditor (parent, option, index, CSMWorld::ColumnBase::Display_Journal);
            }
            case CSMWorld::ConstInfoSelectWrapper::Function_Item:
            {
                return createEditor (parent, option, index, CSMWorld::ColumnBase::Display_Referenceable);
            }
            case CSMWorld::ConstInfoSelectWrapper::Function_Dead:
            case CSMWorld::ConstInfoSelectWrapper::Function_NotId:
            {
                return createEditor (parent, option, index, CSMWorld::ColumnBase::Display_Referenceable);
            }
            case CSMWorld::ConstInfoSelectWrapper::Function_NotFaction:
            {
                return createEditor (parent, option, index, CSMWorld::ColumnBase::Display_Faction);
            }
            case CSMWorld::ConstInfoSelectWrapper::Function_NotClass:
            {
                return createEditor (parent, option, index, CSMWorld::ColumnBase::Display_Class);
            }
            case CSMWorld::ConstInfoSelectWrapper::Function_NotRace:
            {
                return createEditor (parent, option, index, CSMWorld::ColumnBase::Display_Race);
            }
            case CSMWorld::ConstInfoSelectWrapper::Function_NotCell:
            {
                return createEditor (parent, option, index, CSMWorld::ColumnBase::Display_Cell);
            }
            case CSMWorld::ConstInfoSelectWrapper::Function_Local:
            case CSMWorld::ConstInfoSelectWrapper::Function_NotLocal:
            {
                return new CSVWidget::DropLineEdit(display, parent);
            }
            default: return nullptr; // The rest of them can't be edited anyway
        }
    }

    CSMWorld::IdCompletionManager &completionManager = getDocument().getIdCompletionManager();
    CSVWidget::DropLineEdit *editor = new CSVWidget::DropLineEdit(display, parent);
    editor->setCompleter(completionManager.getCompleter(display).get());

    // The savegame format limits the player faction string to 32 characters.
    // The region sound name is limited to 32 characters. (ESM::Region::SoundRef::mSound)
    // The script name is limited to 32 characters. (ESM::Script::SCHD::mName)
    // The cell name is limited to 64 characters. (ESM::Header::GMDT::mCurrentCell)
    if (display == CSMWorld::ColumnBase::Display_Faction ||
        display == CSMWorld::ColumnBase::Display_Sound ||
        display == CSMWorld::ColumnBase::Display_Script ||
        display == CSMWorld::ColumnBase::Display_Referenceable)
    {
        editor->setMaxLength (32);
    }
    else if (display == CSMWorld::ColumnBase::Display_Cell)
    {
        editor->setMaxLength (64);
    }

    return editor;
}

CSVWorld::CommandDelegate *CSVWorld::IdCompletionDelegateFactory::makeDelegate(CSMWorld::CommandDispatcher *dispatcher,
                                                                               CSMDoc::Document& document, 
                                                                               QObject *parent) const
{
    return new IdCompletionDelegate(dispatcher, document, parent);
}
