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
#include <QToolButton>

#include "../../model/world/columnbase.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/record.hpp"
#include "../../model/world/tablemimedata.hpp"
#include "../../model/doc/document.hpp"
#include "../../model/world/commands.hpp"

#include "recordstatusdelegate.hpp"
#include "util.hpp"
#include "tablebottombox.hpp"
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

void CSVWorld::DialogueDelegateDispatcherProxy::tableMimeDataDropped(const std::vector<CSMWorld::UniversalId>& data, const CSMDoc::Document* document)
{
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(mEditor);
    {
        if (!lineEdit || !mIndexWrapper.get())
        {
            return;
        }
    }
    for (unsigned i = 0; i < data.size();  ++i)
    {
        CSMWorld::UniversalId::Type type = data[i].getType();
        if (mDisplay == CSMWorld::ColumnBase::Display_Referenceable)
        {
            if (  type == CSMWorld::UniversalId::Type_Activator
                || type == CSMWorld::UniversalId::Type_Potion
                || type == CSMWorld::UniversalId::Type_Apparatus
                || type == CSMWorld::UniversalId::Type_Armor
                || type == CSMWorld::UniversalId::Type_Book
                || type == CSMWorld::UniversalId::Type_Clothing
                || type == CSMWorld::UniversalId::Type_Container
                || type == CSMWorld::UniversalId::Type_Creature
                || type == CSMWorld::UniversalId::Type_Door
                || type == CSMWorld::UniversalId::Type_Ingredient
                || type == CSMWorld::UniversalId::Type_CreatureLevelledList
                || type == CSMWorld::UniversalId::Type_ItemLevelledList
                || type == CSMWorld::UniversalId::Type_Light
                || type == CSMWorld::UniversalId::Type_Lockpick
                || type == CSMWorld::UniversalId::Type_Miscellaneous
                || type == CSMWorld::UniversalId::Type_Npc
                || type == CSMWorld::UniversalId::Type_Probe
                || type == CSMWorld::UniversalId::Type_Repair
                || type == CSMWorld::UniversalId::Type_Static
                || type == CSMWorld::UniversalId::Type_Weapon)
            {
                type = CSMWorld::UniversalId::Type_Referenceable;
            }
        }
        if (mDisplay == CSMWorld::TableMimeData::convertEnums(type))
        {
            emit tableMimeDataDropped(mEditor, mIndexWrapper->mIndex, data[i], document);
            emit editorDataCommited(mEditor, mIndexWrapper->mIndex, mDisplay);
            break;
        }
    }
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
        mDelegates.insert(std::make_pair(display, delegate));
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
        if (qobject_cast<DropLineEdit*>(editor))
        {
            connect(editor, SIGNAL(editingFinished()), proxy, SLOT(editorDataCommited()));
            connect(editor, SIGNAL(tableMimeDataDropped(const std::vector<CSMWorld::UniversalId>&, const CSMDoc::Document*)),
                    proxy, SLOT(tableMimeDataDropped(const std::vector<CSMWorld::UniversalId>&, const CSMDoc::Document*)));
            connect(proxy, SIGNAL(tableMimeDataDropped(QWidget*, const QModelIndex&, const CSMWorld::UniversalId&, const CSMDoc::Document*)),
                    this, SIGNAL(tableMimeDataDropped(QWidget*, const QModelIndex&, const CSMWorld::UniversalId&, const CSMDoc::Document*)));
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
        if(!skip && qobject_cast<QAbstractSpinBox*>(editor))
        {
            connect(editor, SIGNAL(editingFinished()), proxy, SLOT(editorDataCommited()));
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

CSVWorld::EditWidget::EditWidget(QWidget *parent, int row, CSMWorld::IdTable* table, QUndoStack& undoStack, bool createAndDelete) :
mDispatcher(this, table, undoStack),
QScrollArea(parent),
mWidgetMapper(NULL),
mMainWidget(NULL),
mUndoStack(undoStack),
mTable(table)
{
    remake (row);
    connect(&mDispatcher, SIGNAL(tableMimeDataDropped(QWidget*, const QModelIndex&, const CSMWorld::UniversalId&, const CSMDoc::Document*)), this, SIGNAL(tableMimeDataDropped(QWidget*, const QModelIndex&, const CSMWorld::UniversalId&, const CSMDoc::Document*)));
}

void CSVWorld::EditWidget::remake(int row)
{
    if (mMainWidget)
    {
        delete mMainWidget;
        mMainWidget = 0;
    }
    mMainWidget = new QWidget (this);

    //not sure if widget mapper can handle deleting the widgets that were mapped
    if (mWidgetMapper)
    {
        delete mWidgetMapper;
        mWidgetMapper = 0;
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
    const int columns = mTable->columnCount();
    for (int i=0; i<columns; ++i)
    {
        int flags = mTable->headerData (i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Flags).toInt();

        if (flags & CSMWorld::ColumnBase::Flag_Dialogue)
        {
            CSMWorld::ColumnBase::Display display = static_cast<CSMWorld::ColumnBase::Display>
                (mTable->headerData (i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Display).toInt());

            mDispatcher.makeDelegate(display);
            QWidget *editor = mDispatcher.makeEditor(display, (mTable->index (row, i)));

            if (editor)
            {
                mWidgetMapper->addMapping (editor, i);
                QLabel* label = new QLabel(mTable->headerData (i, Qt::Horizontal).toString(), mMainWidget);
                label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                editor->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);
                if (! (mTable->flags (mTable->index (row, i)) & Qt::ItemIsEditable))
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

    mWidgetMapper->setCurrentModelIndex(mTable->index(row, 0));

    this->setMinimumWidth(325); /// \todo replace hardcoded value with a user setting
    this->setWidget(mMainWidget);
    this->setWidgetResizable(true);
}

/*
==============================DialogueSubView==========================================
*/

CSVWorld::DialogueSubView::DialogueSubView (const CSMWorld::UniversalId& id, CSMDoc::Document& document,
    const CreatorFactoryBase& creatorFactory, bool sorting) :
    SubView (id),
    mEditWidget(0),
    mMainLayout(NULL),
    mUndoStack(document.getUndoStack()),
    mTable(dynamic_cast<CSMWorld::IdTable*>(document.getData().getTableModel(id))),
    mRow (-1),
    mLocked(false),
    mDocument(document),
    mCommandDispatcher (document, CSMWorld::UniversalId::getParentType (id.getType()))
{
    connect(mTable, SIGNAL(dataChanged (const QModelIndex&, const QModelIndex&)), this, SLOT(dataChanged(const QModelIndex&)));
    mRow = mTable->getModelIndex (id.getId(), 0).row();
    QWidget *mainWidget = new QWidget(this);

    QHBoxLayout *buttonsLayout = new QHBoxLayout;
    QToolButton* prevButton = new QToolButton(mainWidget);
    prevButton->setIcon(QIcon(":/go-previous.png"));
    prevButton->setToolTip ("Switch to previous record");
    QToolButton* nextButton = new QToolButton(mainWidget);
    nextButton->setIcon(QIcon(":/go-next.png"));
    nextButton->setToolTip ("Switch to next record");
    buttonsLayout->addWidget(prevButton, 0);
    buttonsLayout->addWidget(nextButton, 1);
    buttonsLayout->addStretch(2);

    QToolButton* cloneButton = new QToolButton(mainWidget);
    cloneButton->setIcon(QIcon(":/edit-clone.png"));
    cloneButton->setToolTip ("Clone record");
    QToolButton* addButton = new QToolButton(mainWidget);
    addButton->setIcon(QIcon(":/add.png"));
    addButton->setToolTip ("Add new record");
    QToolButton* deleteButton = new QToolButton(mainWidget);
    deleteButton->setIcon(QIcon(":/edit-delete.png"));
    deleteButton->setToolTip ("Delete record");
    QToolButton* revertButton = new QToolButton(mainWidget);
    revertButton->setIcon(QIcon(":/edit-undo.png"));
    revertButton->setToolTip ("Revert record");

    if (mTable->getFeatures() & CSMWorld::IdTable::Feature_Preview)
    {
        QToolButton* previewButton = new QToolButton(mainWidget);
        previewButton->setIcon(QIcon(":/edit-preview.png"));
        previewButton->setToolTip ("Open a preview of this record");
        buttonsLayout->addWidget(previewButton);
        connect(previewButton, SIGNAL(clicked()), this, SLOT(showPreview()));
    }

    if (mTable->getFeatures() & CSMWorld::IdTable::Feature_View)
    {
        QToolButton* viewButton = new QToolButton(mainWidget);
        viewButton->setIcon(QIcon(":/cell.png"));
        viewButton->setToolTip ("Open a scene view of the cell this record is located in");
        buttonsLayout->addWidget(viewButton);
        connect(viewButton, SIGNAL(clicked()), this, SLOT(viewRecord()));
    }

    buttonsLayout->addWidget(cloneButton);
    buttonsLayout->addWidget(addButton);
    buttonsLayout->addWidget(deleteButton);
    buttonsLayout->addWidget(revertButton);

    connect(nextButton, SIGNAL(clicked()), this, SLOT(nextId()));
    connect(prevButton, SIGNAL(clicked()), this, SLOT(prevId()));
    connect(cloneButton, SIGNAL(clicked()), this, SLOT(cloneRequest()));
    connect(revertButton, SIGNAL(clicked()), this, SLOT(revertRecord()));
    connect(deleteButton, SIGNAL(clicked()), this, SLOT(deleteRecord()));

    mMainLayout = new QVBoxLayout(mainWidget);

    mEditWidget = new EditWidget(mainWidget, mRow, mTable, mUndoStack, false);
    connect(mEditWidget, SIGNAL(tableMimeDataDropped(QWidget*, const QModelIndex&, const CSMWorld::UniversalId&, const CSMDoc::Document*)),
            this, SLOT(tableMimeDataDropped(QWidget*, const QModelIndex&, const CSMWorld::UniversalId&, const CSMDoc::Document*)));

    mMainLayout->addWidget(mEditWidget);
    mEditWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    mMainLayout->addWidget (mBottom =
        new TableBottomBox (creatorFactory, document.getData(), document.getUndoStack(), id, this));

    mBottom->setSizePolicy(QSizePolicy::Ignored, QSizePolicy::Fixed);
    connect(mBottom, SIGNAL(requestFocus(const std::string&)), this, SLOT(requestFocus(const std::string&)));
    connect(addButton, SIGNAL(clicked()), mBottom, SLOT(createRequest()));

    if(!mBottom->canCreateAndDelete())
    {
        cloneButton->setDisabled(true);
        addButton->setDisabled(true);
        deleteButton->setDisabled(true);
    }

    dataChanged(mTable->index(mRow, 0));
    mMainLayout->addLayout(buttonsLayout);
    setWidget(mainWidget);
}

void CSVWorld::DialogueSubView::prevId()
{
    int newRow = mRow - 1;
    if (newRow < 0)
    {
        return;
    }
    while (newRow >= 0)
    {
        QModelIndex newIndex(mTable->index(newRow, 0));

        if (!newIndex.isValid())
        {
            return;
        }

        CSMWorld::RecordBase::State state = static_cast<CSMWorld::RecordBase::State>(mTable->data (mTable->index (newRow, 1)).toInt());
        if (!(state == CSMWorld::RecordBase::State_Deleted || state == CSMWorld::RecordBase::State_Erased))
        {
                mEditWidget->remake(newRow);
                setUniversalId(CSMWorld::UniversalId (static_cast<CSMWorld::UniversalId::Type> (mTable->data (mTable->index (newRow, 2)).toInt()),
                                        mTable->data (mTable->index (newRow, 0)).toString().toUtf8().constData()));
                mRow = newRow;
                mEditWidget->setDisabled(mLocked);
                return;
        }
        --newRow;
    }
}

void CSVWorld::DialogueSubView::nextId()
{
    int newRow = mRow + 1;

    if (newRow >= mTable->rowCount())
    {
        return;
    }

    while (newRow < mTable->rowCount())
    {
        QModelIndex newIndex(mTable->index(newRow, 0));

        if (!newIndex.isValid())
        {
            return;
        }

        CSMWorld::RecordBase::State state = static_cast<CSMWorld::RecordBase::State>(mTable->data (mTable->index (newRow, 1)).toInt());
        if (!(state == CSMWorld::RecordBase::State_Deleted || state == CSMWorld::RecordBase::State_Erased))
        {
                mEditWidget->remake(newRow);
                setUniversalId(CSMWorld::UniversalId (static_cast<CSMWorld::UniversalId::Type> (mTable->data (mTable->index (newRow, 2)).toInt()),
                                          mTable->data (mTable->index (newRow, 0)).toString().toUtf8().constData()));
                mRow = newRow;
                mEditWidget->setDisabled(mLocked);
                return;
        }
        ++newRow;
    }
}

void CSVWorld::DialogueSubView::setEditLock (bool locked)
{
    mLocked = locked;

    CSMWorld::RecordBase::State state = static_cast<CSMWorld::RecordBase::State>(mTable->data (mTable->index (mRow, 1)).toInt());

    mEditWidget->setDisabled (state==CSMWorld::RecordBase::State_Deleted || locked);

    mCommandDispatcher.setEditLock (locked);
}

void CSVWorld::DialogueSubView::dataChanged(const QModelIndex & index)
{
    if (index.row() == mRow)
    {
        CSMWorld::RecordBase::State state = static_cast<CSMWorld::RecordBase::State>(mTable->data (mTable->index (mRow, 1)).toInt());

        mEditWidget->setDisabled (state==CSMWorld::RecordBase::State_Deleted || mLocked);
    }
}

void CSVWorld::DialogueSubView::tableMimeDataDropped(QWidget* editor,
                                                     const QModelIndex& index,
                                                     const CSMWorld::UniversalId& id,
                                                     const CSMDoc::Document* document)
{
    if (document == &mDocument)
    {
        qobject_cast<DropLineEdit*>(editor)->setText(id.getId().c_str());
    }
}

void CSVWorld::DialogueSubView::revertRecord()
{
    int rows = mTable->rowCount();
    if (!mLocked && mTable->columnCount() > 0 && mRow < mTable->rowCount() )
    {
         CSMWorld::RecordBase::State state =
                static_cast<CSMWorld::RecordBase::State> (mTable->data (mTable->index (mRow, 1)).toInt());

        if (state!=CSMWorld::RecordBase::State_BaseOnly)
        {
            mUndoStack.push(new CSMWorld::RevertCommand(*mTable, mTable->data(mTable->index (mRow, 0)).toString().toUtf8().constData()));
        }
        if (rows != mTable->rowCount())
        {
            if (mTable->rowCount() == 0)
            {
                mEditWidget->setDisabled(true); //closing the editor is other option
                return;
            }
            if (mRow >= mTable->rowCount())
            {
                prevId();
            } else {
                dataChanged(mTable->index(mRow, 0));
            }
        }
    }
}

void CSVWorld::DialogueSubView::deleteRecord()
{
    int rows = mTable->rowCount();

    //easier than disabling the button
    CSMWorld::RecordBase::State state = static_cast<CSMWorld::RecordBase::State>(mTable->data (mTable->index (mRow, 1)).toInt());
    bool deledetedOrErased = (state == CSMWorld::RecordBase::State_Deleted || state == CSMWorld::RecordBase::State_Erased);

    if (!mLocked &&
        mTable->columnCount() > 0 &&
        !deledetedOrErased &&
        mRow < rows &&
        mBottom->canCreateAndDelete())
    {
        mUndoStack.push(new CSMWorld::DeleteCommand(*mTable, mTable->data(mTable->index (mRow, 0)).toString().toUtf8().constData()));
        if (rows != mTable->rowCount())
        {
            if (mTable->rowCount() == 0)
            {
                mEditWidget->setDisabled(true); //closing the editor is other option
                return;
            }
            if (mRow >= mTable->rowCount())
            {
                prevId();
            } else {
                dataChanged(mTable->index(mRow, 0));
            }
        }
    }
}

void CSVWorld::DialogueSubView::requestFocus (const std::string& id)
{
    mRow = mTable->getModelIndex (id, 0).row();
    mEditWidget->remake(mRow);
}

void CSVWorld::DialogueSubView::cloneRequest ()
{
    mBottom->cloneRequest(mTable->data(mTable->index (mRow, 0)).toString().toUtf8().constData(),
                          static_cast<CSMWorld::UniversalId::Type>(mTable->data(mTable->index(mRow, 2)).toInt()));
}

void CSVWorld::DialogueSubView::showPreview ()
{
    if ((mTable->getFeatures() & CSMWorld::IdTable::Feature_Preview) &&
        mRow < mTable->rowCount())
    {
       emit focusId(CSMWorld::UniversalId(CSMWorld::UniversalId::Type_Preview, mTable->data(mTable->index (mRow, 0)).toString().toUtf8().constData()), "");
    }
}

void CSVWorld::DialogueSubView::viewRecord()
{
    if (mRow < mTable->rowCount())
    {
        std::pair<CSMWorld::UniversalId, std::string> params = mTable->view (mRow);

        if (params.first.getType()!=CSMWorld::UniversalId::Type_None)
            emit focusId (params.first, params.second);
    }
}
