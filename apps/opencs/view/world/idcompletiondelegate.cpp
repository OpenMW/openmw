#include "idcompletiondelegate.hpp"

#include "../../model/world/idcompletionmanager.hpp"

CSVWorld::IdCompletionDelegate::IdCompletionDelegate(CSMWorld::CommandDispatcher *dispatcher,
                                                     CSMDoc::Document& document,
                                                     QObject *parent)
    : CommandDelegate(dispatcher, document, parent)
{}

QWidget *CSVWorld::IdCompletionDelegate::createEditor(QWidget *parent,
                                                      const QStyleOptionViewItem &option,
                                                      const QModelIndex &index) const
{
    return createEditor(parent, option, index, CSMWorld::ColumnBase::Display_None);
}

QWidget *CSVWorld::IdCompletionDelegate::createEditor(QWidget *parent,
                                                      const QStyleOptionViewItem &option,
                                                      const QModelIndex &index,
                                                      CSMWorld::ColumnBase::Display display) const
{
    int columnIdData = index.data(CSMWorld::ColumnBase::Role_ColumnId).toInt();
    CSMWorld::Columns::ColumnId columnId = static_cast<CSMWorld::Columns::ColumnId>(columnIdData);
    CSMWorld::IdCompletionManager &completionManager = getDocument().getIdCompletionManager();

    QWidget *editor = CSVWorld::CommandDelegate::createEditor(parent, option, index, display);
    QLineEdit *lineEditor = qobject_cast<QLineEdit *>(editor);
    if (lineEditor != NULL && completionManager.isCompleterExistFor(columnId))
    {
        lineEditor->setCompleter(completionManager.getCompleter(columnId).get());
    }
    return editor;
}

CSVWorld::CommandDelegate *CSVWorld::IdCompletionDelegateFactory::makeDelegate(CSMWorld::CommandDispatcher *dispatcher,
                                                                               CSMDoc::Document& document, 
                                                                               QObject *parent) const
{
    return new IdCompletionDelegate(dispatcher, document, parent);
}
