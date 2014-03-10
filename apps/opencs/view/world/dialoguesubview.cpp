
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
#include "../../model/world/columns.hpp"

#include "recordstatusdelegate.hpp"
#include "util.hpp"

/*
==============================DialogueDelegateDispatcherProxy==========================================
*/
CSVWorld::DialogueDelegateDispatcherProxy::refWrapper::refWrapper(const QModelIndex& index) :
mIndex(index)
{}

CSVWorld::DialogueDelegateDispatcherProxy::DialogueDelegateDispatcherProxy(QWidget* editor, CSMWorld::ColumnBase::Display display) :
mEditor(editor),
mDisplay(display),
mIndexWrapper(NULL)
{
}

void CSVWorld::DialogueDelegateDispatcherProxy::editorDataCommited()
{
    if (mIndexWrapper.get())
    {
        emit editorDataCommited(mEditor, mIndexWrapper->mIndex, mDisplay);
    }
}

void CSVWorld::DialogueDelegateDispatcherProxy::setIndex(const QModelIndex& index)
{
    mIndexWrapper.reset(new refWrapper(index));
}

QWidget* CSVWorld::DialogueDelegateDispatcherProxy::getEditor() const
{
    return mEditor;
}

/*
==============================DialogueDelegateDispatcher==========================================
*/

CSVWorld::DialogueDelegateDispatcher::DialogueDelegateDispatcher(QObject* parent, CSMWorld::IdTable* table, QUndoStack& undoStack) :
mParent(parent),
mTable(table),
mUndoStack(undoStack)
{
}

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
    return delegate;
}

void CSVWorld::DialogueDelegateDispatcher::editorDataCommited(QWidget* editor, const QModelIndex& index, CSMWorld::ColumnBase::Display display)
{
    setModelData(editor, mTable, index, display);
}

void CSVWorld::DialogueDelegateDispatcher::setEditorData (QWidget* editor, const QModelIndex& index) const
{
    CSMWorld::ColumnBase::Display display = static_cast<CSMWorld::ColumnBase::Display>
    (mTable->headerData (index.column(), Qt::Horizontal, CSMWorld::ColumnBase::Role_Display).toInt());

    std::map<int, CommandDelegate*>::const_iterator delegateIt(mDelegates.find(display));
    if (delegateIt != mDelegates.end())
    {
        delegateIt->second->setEditorData(editor, index, true);
    }

    for (unsigned i = 0; i < mProxys.size(); ++i)
    {
       if (mProxys[i]->getEditor() == editor)
        {
            mProxys[i]->setIndex(index);
        }
    }
}

void CSVWorld::DialogueDelegateDispatcher::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index, CSMWorld::ColumnBase::Display display) const
{
    std::map<int, CommandDelegate*>::const_iterator delegateIt(mDelegates.find(display));
    if (delegateIt != mDelegates.end())
    {
        delegateIt->second->setModelData(editor, model, index);
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
    bool hasEnums = CSMWorld::Columns::hasEnums(static_cast<CSMWorld::Columns::ColumnId>(mTable->getColumnId(index.column() ) ) );
    if (display == CSMWorld::ColumnBase::Display_Boolean)
    {
        hasEnums = true;
    }

    QWidget* editor = NULL;
    std::map<int, CommandDelegate*>::iterator delegateIt(mDelegates.find(display));
    if (delegateIt != mDelegates.end())
    {
        editor = delegateIt->second->createEditor(dynamic_cast<QWidget*>(mParent), QStyleOptionViewItem(), index);
        DialogueDelegateDispatcherProxy* proxy = new DialogueDelegateDispatcherProxy(editor, display);
        if (hasEnums) //combox is used for all enums
        {
            connect(editor, SIGNAL(currentIndexChanged (int)), proxy, SLOT(editorDataCommited()));
        } else
        {
            connect(editor, SIGNAL(editingFinished()), proxy, SLOT(editorDataCommited()));
        }
        connect(proxy, SIGNAL(editorDataCommited(QWidget*, const QModelIndex&, CSMWorld::ColumnBase::Display)), this, SLOT(editorDataCommited(QWidget*, const QModelIndex&, CSMWorld::ColumnBase::Display)));
        mProxys.push_back(proxy); //deleted in the destructor
    }
    return editor;
}

CSVWorld::DialogueDelegateDispatcher::~DialogueDelegateDispatcher()
{
    for (unsigned i = 0; i < mProxys.size(); ++i)
    {
        delete mProxys[i]; //unique_ptr could be handy
    }
}

/*
==============================DialogueSubView==========================================
*/

CSVWorld::DialogueSubView::DialogueSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document,
    bool createAndDelete) :

    SubView (id),
    mDispatcher(this, dynamic_cast<CSMWorld::IdTable*>(document.getData().getTableModel (id)), document.getUndoStack())

{
    QWidget *widget = new QWidget (this);

    setWidget (widget);

    QFrame* line = new QFrame(this);
    line->setObjectName(QString::fromUtf8("line"));
    line->setGeometry(QRect(320, 150, 118, 3));
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    QVBoxLayout *mainLayout = new QVBoxLayout;
    QGridLayout *unlockedLayout = new QGridLayout;
    QGridLayout *lockedLayout = new QGridLayout;
    mainLayout->addLayout(lockedLayout, 0);
    mainLayout->addWidget(line, 1);
    mainLayout->addLayout(unlockedLayout, 2);
    mainLayout->addStretch(1);

    widget->setLayout (mainLayout);

    QAbstractItemModel *model = document.getData().getTableModel (id);

    int columns = model->columnCount();

    mWidgetMapper = new QDataWidgetMapper (this);
    mWidgetMapper->setModel (model);
    mWidgetMapper->setItemDelegate(&mDispatcher);

    int unlocked = 0;
    int locked = 0;
    std::vector<QWidget*> editors;
    for (int i=0; i<columns; ++i)
    {
        int flags = model->headerData (i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Flags).toInt();

        if (flags & CSMWorld::ColumnBase::Flag_Dialogue)
        {
            CSMWorld::ColumnBase::Display display = static_cast<CSMWorld::ColumnBase::Display>
                (model->headerData (i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Display).toInt());

            mDispatcher.makeDelegate(display);
            QWidget *editor = mDispatcher.makeEditor(display, (model->index (0, i)));

            if (editor)
            {
                editors.push_back(editor);
                mWidgetMapper->addMapping (editor, i);
                QLabel* label = new QLabel(model->headerData (i, Qt::Horizontal).toString());
                label->setSizePolicy(QSizePolicy::Maximum, QSizePolicy::Fixed);
                editor->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
                if (! (model->flags (model->index (0, i)) & Qt::ItemIsEditable))
                {
                    editor->setDisabled(true);
                    lockedLayout->addWidget (label, locked, 0);
                    lockedLayout->addWidget (editor, locked, 1);
                    ++locked;
                } else
                {
                    unlockedLayout->addWidget (label, unlocked, 0);
                    unlockedLayout->addWidget (editor, unlocked, 1);
                    ++unlocked;
                }
            }
        }
    }

    mWidgetMapper->setCurrentModelIndex (
        dynamic_cast<CSMWorld::IdTable&> (*model).getModelIndex (id.getId(), 0));
}

void CSVWorld::DialogueSubView::setEditLock (bool locked)
{

}