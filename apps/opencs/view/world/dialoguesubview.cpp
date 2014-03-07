
#include "dialoguesubview.hpp"

#include <utility>
#include <memory>

#include <QGridLayout>
#include <QLabel>
#include <QSize>
#include <QAbstractItemModel>
#include <QDoubleSpinBox>
#include <QSpinBox>
#include <QLineEdit>
#include <QEvent>
#include <QDataWidgetMapper>

#include "../../model/world/columnbase.hpp"
#include "../../model/world/idtable.hpp"

#include "recordstatusdelegate.hpp"
#include "util.hpp"

CSVWorld::DialogueDelegateDispatcher::DialogueDelegateDispatcher(QObject* parent, CSMWorld::IdTable* table, QUndoStack& undoStack) :
mParent(parent),
mTable(table),
mUndoStack(undoStack)
{}

CSVWorld::CommandDelegate* CSVWorld::DialogueDelegateDispatcher::makeDelegate(CSMWorld::ColumnBase::Display display)
{
    CommandDelegate *delegate = NULL;
    std::map<int, CommandDelegate*>::const_iterator delegateIt(mDelegates.find(display));
    if (delegateIt == mDelegates.end())
    {
        delegate = CommandDelegateFactoryCollection::get().makeDelegate (
                                    display, mUndoStack, mParent);
        mDelegates.insert(std::make_pair<int, CommandDelegate*>(display, delegate));
    } else
    {
        delegate = delegateIt->second;
    }
    connect(this, SIGNAL(closeEditor(QWidget *)), this, SLOT(editorDataCommited(QWidget*)));
    return delegate;
}

void CSVWorld::DialogueDelegateDispatcher::editorDataCommited( QWidget * editor )
{
     std::cout<<"triggered"<<std::endl;
}

void CSVWorld::DialogueDelegateDispatcher::setEditorData (QWidget* editor, const QModelIndex& index) const
{
    CSMWorld::ColumnBase::Display display = static_cast<CSMWorld::ColumnBase::Display>
    (mTable->headerData (index.column(), Qt::Horizontal, CSMWorld::ColumnBase::Role_Display).toInt());

    std::map<int, CommandDelegate*>::const_iterator delegateIt(mDelegates.find(display));
    if (delegateIt != mDelegates.end())
    {
        delegateIt->second->setEditorData(editor, index);
    }
}

void CSVWorld::DialogueDelegateDispatcher::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    CSMWorld::ColumnBase::Display display = static_cast<CSMWorld::ColumnBase::Display>
    (mTable->headerData (index.column(), Qt::Horizontal, CSMWorld::ColumnBase::Role_Display).toInt());

    std::cout<<"setting data\n";
    std::map<int, CommandDelegate*>::const_iterator delegateIt(mDelegates.find(display));
    if (delegateIt != mDelegates.end())
    {
        delegateIt->second->setModelData(editor, model, index);
    } else {
        std::cout<<"oooops\n";
    }
}

void CSVWorld::DialogueDelegateDispatcher::paint (QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    //Does nothing
}

QSize CSVWorld::DialogueDelegateDispatcher::sizeHint (const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return QSize(); //silencing warning, otherwise does nothing
}

QWidget* CSVWorld::DialogueDelegateDispatcher::makeEditor(CSMWorld::ColumnBase::Display display, const QModelIndex& index)
{
    QWidget* editor = NULL;
    std::map<int, CommandDelegate*>::iterator delegateIt(mDelegates.find(display));
    if (delegateIt != mDelegates.end())
    {
        editor = delegateIt->second->createEditor(dynamic_cast<QWidget*>(mParent), QStyleOptionViewItem(), index);
    }
    return editor;
}

CSVWorld::DialogueSubView::DialogueSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document,
    bool createAndDelete) :

    SubView (id),
    mDispatcher(new DialogueDelegateDispatcher(this, dynamic_cast<CSMWorld::IdTable*>(document.getData().getTableModel (id)), document.getUndoStack()))

{
    QWidget *widget = new QWidget (this);

    setWidget (widget);

    QGridLayout *layout = new QGridLayout;

    widget->setLayout (layout);

    QAbstractItemModel *model = document.getData().getTableModel (id);

    int columns = model->columnCount();

    mWidgetMapper = new QDataWidgetMapper (this);
    mWidgetMapper->setModel (model);
    mWidgetMapper->setItemDelegate(mDispatcher.get());

    for (int i=0; i<columns; ++i)
    {
        int flags = model->headerData (i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Flags).toInt();

        if (flags & CSMWorld::ColumnBase::Flag_Dialogue)
        {
            layout->addWidget (new QLabel (model->headerData (i, Qt::Horizontal).toString()), i, 0);

            CSMWorld::ColumnBase::Display display = static_cast<CSMWorld::ColumnBase::Display>
                (model->headerData (i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Display).toInt());

            mDispatcher->makeDelegate(display);
            QWidget *widget = mDispatcher->makeEditor(display, (model->index (0, i)));

            if (widget)
            {
                layout->addWidget (widget, i, 1);
                mWidgetMapper->addMapping (widget, i);
            }

            if (model->flags (model->index (0, i)) & Qt::ItemIsEditable)
            {

            }
        }
    }

    mWidgetMapper->setCurrentModelIndex (
        dynamic_cast<CSMWorld::IdTable&> (*model).getModelIndex (id.getId(), 0));
}

void CSVWorld::DialogueSubView::setEditLock (bool locked)
{

}