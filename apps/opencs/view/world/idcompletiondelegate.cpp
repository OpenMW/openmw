#include "idcompletiondelegate.hpp"

#include "../../model/world/idcompletionmanager.hpp"

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
        return NULL;
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
