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
    return editor;
}

CSVWorld::CommandDelegate *CSVWorld::IdCompletionDelegateFactory::makeDelegate(CSMWorld::CommandDispatcher *dispatcher,
                                                                               CSMDoc::Document& document, 
                                                                               QObject *parent) const
{
    return new IdCompletionDelegate(dispatcher, document, parent);
}
