
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
#include <QCheckBox>
#include <QLineEdit>
#include <QPlainTextEdit>
#include <QComboBox>
#include <QScrollArea>
#include <QPushButton>

#include "../../model/world/columnbase.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/columns.hpp"

#include "recordstatusdelegate.hpp"
#include "util.hpp"
/*
==============================NotEditableSubDelegate==========================================
*/
CSVWorld::NotEditableSubDelegate::NotEditableSubDelegate(const CSMWorld::IdTable* table, QObject * parent) :
QAbstractItemDelegate(parent),
mTable(table)
{}

void CSVWorld::NotEditableSubDelegate::setEditorData (QLabel* editor, const QModelIndex& index) const
{
    QVariant v = index.data(Qt::EditRole);
    if (!v.isValid())
    {
        v = index.data(Qt::DisplayRole);
        if (!v.isValid())
        {
            return;
        }
    }

    if (QVariant::String == v.type())
    {
        editor->setText(v.toString());
    } else //else we are facing enums
    {
        int data = v.toInt();
        std::vector<std::string> enumNames (CSMWorld::Columns::getEnums (static_cast<CSMWorld::Columns::ColumnId> (mTable->getColumnId (index.column()))));
        editor->setText(QString::fromUtf8(enumNames.at(data).c_str()));
    }
}

void CSVWorld::NotEditableSubDelegate::setModelData (QWidget* editor, QAbstractItemModel* model, const QModelIndex& index, CSMWorld::ColumnBase::Display display) const
{
    //not editable widgets will not save model data
}

void CSVWorld::NotEditableSubDelegate::paint (QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    //does nothing
}

QSize CSVWorld::NotEditableSubDelegate::sizeHint (const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return QSize();
}

QWidget* CSVWorld::NotEditableSubDelegate::createEditor (QWidget *parent,
                                const QStyleOptionViewItem& option,
                                const QModelIndex& index,
                                CSMWorld::ColumnBase::Display display) const
{
    return new QLabel(parent);
}

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
mUndoStack(undoStack),
mNotEditableDelegate(table, parent)
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

    QLabel* label = qobject_cast<QLabel*>(editor);
    if(label)
    {
        mNotEditableDelegate.setEditorData(label, index);
        return;
    }

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
    QVariant variant = index.data();
    if (!variant.isValid())
    {
        variant = index.data(Qt::DisplayRole);
        if (!variant.isValid())
        {
            return NULL;
        }
    }

    QWidget* editor = NULL;
    if (! (mTable->flags (index) & Qt::ItemIsEditable))
    {
        return mNotEditableDelegate.createEditor(qobject_cast<QWidget*>(mParent), QStyleOptionViewItem(), index, display);
    }

    std::map<int, CommandDelegate*>::iterator delegateIt(mDelegates.find(display));
    if (delegateIt != mDelegates.end())
    {
        editor = delegateIt->second->createEditor(qobject_cast<QWidget*>(mParent), QStyleOptionViewItem(), index, display);
        DialogueDelegateDispatcherProxy* proxy = new DialogueDelegateDispatcherProxy(editor, display);

        bool skip = false;
        if (qobject_cast<QLineEdit*>(editor))
        {
            connect(editor, SIGNAL(editingFinished()), proxy, SLOT(editorDataCommited()));
            skip = true;
        }
        if(!skip && qobject_cast<QCheckBox*>(editor))
        {
            connect(editor, SIGNAL(stateChanged(int)), proxy, SLOT(editorDataCommited()));
            skip = true;
        }
        if(!skip && qobject_cast<QPlainTextEdit*>(editor))
        {
            connect(editor, SIGNAL(textChanged()), proxy, SLOT(editorDataCommited()));
            skip = true;
        }
        if(!skip && qobject_cast<QComboBox*>(editor))
        {
            connect(editor, SIGNAL(currentIndexChanged (int)), proxy, SLOT(editorDataCommited()));
            skip = true;
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
=============================================================EditWidget=====================================================
*/

CSVWorld::EditWidget::EditWidget(QWidget *parent, const CSMWorld::UniversalId& id, CSMDoc::Document& document, bool createAndDelete) :
mDispatcher(this, dynamic_cast<CSMWorld::IdTable*>(document.getData().getTableModel (id)), document.getUndoStack()),
QScrollArea(parent),
mWidgetMapper(NULL),
mMainWidget(NULL),
mUndoStack(document.getUndoStack()),
mTable(dynamic_cast<CSMWorld::IdTable*>(document.getData().getTableModel(id)))
{
    remake (id);
}

void CSVWorld::EditWidget::remake(const CSMWorld::UniversalId& id)
{
    const QModelIndex indexToFocus(mTable->getModelIndex (id.getId(), 0));

    if (mMainWidget)
    {
        delete mMainWidget;
    }
    mMainWidget = new QWidget (this);

    if (mWidgetMapper)
    {
        delete mWidgetMapper;
    }
    mWidgetMapper = new QDataWidgetMapper (this);
    mWidgetMapper->setModel(mTable);
    mWidgetMapper->setItemDelegate(&mDispatcher);

    QFrame* line = new QFrame(mMainWidget);
    line->setObjectName(QString::fromUtf8("line"));
    line->setGeometry(QRect(320, 150, 118, 3));
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    QVBoxLayout *mainLayout = new QVBoxLayout(mMainWidget);
    QGridLayout *unlockedLayout = new QGridLayout();
    QGridLayout *lockedLayout = new QGridLayout();
    mainLayout->addLayout(lockedLayout, 0);
    mainLayout->addWidget(line, 1);
    mainLayout->addLayout(unlockedLayout, 2);
    mainLayout->addStretch(1);

    int unlocked = 0;
    int locked = 0;
    const int focusedRow = indexToFocus.row();
    const int columns = mTable->columnCount();
    for (int i=0; i<columns; ++i)
    {
        int flags = mTable->headerData (i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Flags).toInt();

        if (flags & CSMWorld::ColumnBase::Flag_Dialogue)
        {
            CSMWorld::ColumnBase::Display display = static_cast<CSMWorld::ColumnBase::Display>
                (mTable->headerData (i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Display).toInt());

            mDispatcher.makeDelegate(display);
            QWidget *editor = mDispatcher.makeEditor(display, (mTable->index (focusedRow, i)));

            if (editor)
            {
                mWidgetMapper->addMapping (editor, i);
                QLabel* label = new QLabel(mTable->headerData (i, Qt::Horizontal).toString(), mMainWidget);
                label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                editor->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
                if (! (mTable->flags (mTable->index (0, i)) & Qt::ItemIsEditable))
                {
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

    mWidgetMapper->setCurrentModelIndex (indexToFocus);

    this->setMinimumWidth(300);
    this->setWidget(mMainWidget);
    this->setWidgetResizable(true);
}

/*
==============================DialogueSubView==========================================
*/

CSVWorld::DialogueSubView::DialogueSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document,
    bool createAndDelete) :

    SubView (id)

{
    QWidget *mainWidget = new QWidget(this);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    QPushButton* mPrevButton = new QPushButton(tr("Previous"));
    QPushButton* mNextButton = new QPushButton(tr("Next"));
    buttonsLayout->addWidget(mPrevButton);
    buttonsLayout->addWidget(mNextButton);

    QVBoxLayout *mainLayout = new QVBoxLayout(mainWidget);

    EditWidget* editWidget = new EditWidget(mainWidget, id, document, false);
    mainLayout->addLayout(buttonsLayout);
    mainLayout->addWidget(editWidget);
    editWidget->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Ignored);
    setWidget(mainWidget);

}

void CSVWorld::DialogueSubView::setEditLock (bool locked)
{

}