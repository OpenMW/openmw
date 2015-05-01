#include "dialoguesubview.hpp"

#include <utility>
#include <memory>
#include <stdexcept>

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
#include <QPushButton>
#include <QToolButton>
#include <QHeaderView>

#include "../../model/world/nestedtableproxymodel.hpp"
#include "../../model/world/columnbase.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/idtree.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/record.hpp"
#include "../../model/world/tablemimedata.hpp"
#include "../../model/world/idtree.hpp"
#include "../../model/world/commands.hpp"
#include "../../model/doc/document.hpp"

#include "recordstatusdelegate.hpp"
#include "util.hpp"
#include "tablebottombox.hpp"
#include "nestedtable.hpp"
/*
==============================NotEditableSubDelegate==========================================
*/
CSVWorld::NotEditableSubDelegate::NotEditableSubDelegate(const CSMWorld::IdTable* table, QObject * parent) :
QAbstractItemDelegate(parent),
mTable(table)
{}

void CSVWorld::NotEditableSubDelegate::setEditorData (QWidget* editor, const QModelIndex& index) const
{
    QLabel* label = qobject_cast<QLabel*>(editor);
    if(!label)
        return;

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
        label->setText(v.toString());
    }
    else //else we are facing enums
    {
        int data = v.toInt();
        std::vector<std::string> enumNames (CSMWorld::Columns::getEnums (static_cast<CSMWorld::Columns::ColumnId> (mTable->getColumnId (index.column()))));
        label->setText(QString::fromUtf8(enumNames.at(data).c_str()));
    }
}

void CSVWorld::NotEditableSubDelegate::setModelData(QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
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
                                const QModelIndex& index) const
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
            if (type == CSMWorld::UniversalId::Type_Activator
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

CSVWorld::DialogueDelegateDispatcher::DialogueDelegateDispatcher(QObject* parent,
        CSMWorld::IdTable* table, CSMWorld::CommandDispatcher& commandDispatcher,
        CSMDoc::Document& document, QAbstractItemModel *model) :
mParent(parent),
mTable(model ? model : table),
mCommandDispatcher (commandDispatcher), mDocument (document),
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
                                    display, &mCommandDispatcher, mDocument, mParent);
        mDelegates.insert(std::make_pair(display, delegate));
    } else
    {
        delegate = delegateIt->second;
    }
    return delegate;
}

void CSVWorld::DialogueDelegateDispatcher::editorDataCommited(QWidget* editor,
        const QModelIndex& index, CSMWorld::ColumnBase::Display display)
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

void CSVWorld::DialogueDelegateDispatcher::setModelData(QWidget* editor,
        QAbstractItemModel* model, const QModelIndex& index) const
{
    setModelData(editor, model, index, CSMWorld::ColumnBase::Display_None);
}

void CSVWorld::DialogueDelegateDispatcher::setModelData(QWidget* editor,
        QAbstractItemModel* model, const QModelIndex& index, CSMWorld::ColumnBase::Display display) const
{
    std::map<int, CommandDelegate*>::const_iterator delegateIt(mDelegates.find(display));
    if (delegateIt != mDelegates.end())
    {
        delegateIt->second->setModelData(editor, model, index);
    }
}

void CSVWorld::DialogueDelegateDispatcher::paint (QPainter* painter,
        const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    //Does nothing
}

QSize CSVWorld::DialogueDelegateDispatcher::sizeHint (const QStyleOptionViewItem& option,
        const QModelIndex& index) const
{
    return QSize(); //silencing warning, otherwise does nothing
}

QWidget* CSVWorld::DialogueDelegateDispatcher::makeEditor(CSMWorld::ColumnBase::Display display,
        const QModelIndex& index)
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
        return mNotEditableDelegate.createEditor(qobject_cast<QWidget*>(mParent),
                QStyleOptionViewItem(), index);
    }

    std::map<int, CommandDelegate*>::iterator delegateIt(mDelegates.find(display));

    if (delegateIt != mDelegates.end())
    {
        editor = delegateIt->second->createEditor(qobject_cast<QWidget*>(mParent),
                QStyleOptionViewItem(), index, display);

        DialogueDelegateDispatcherProxy* proxy = new DialogueDelegateDispatcherProxy(editor, display);

        // NOTE: For each entry in CSVWorld::CommandDelegate::createEditor() a corresponding entry
        // is required here
        if (qobject_cast<DropLineEdit*>(editor))
        {
            connect(editor, SIGNAL(editingFinished()), proxy, SLOT(editorDataCommited()));

            connect(editor, SIGNAL(tableMimeDataDropped(const std::vector<CSMWorld::UniversalId>&, const CSMDoc::Document*)),
                    proxy, SLOT(tableMimeDataDropped(const std::vector<CSMWorld::UniversalId>&, const CSMDoc::Document*)));

            connect(proxy, SIGNAL(tableMimeDataDropped(QWidget*, const QModelIndex&, const CSMWorld::UniversalId&, const CSMDoc::Document*)),
                    this, SIGNAL(tableMimeDataDropped(QWidget*, const QModelIndex&, const CSMWorld::UniversalId&, const CSMDoc::Document*)));

        }
        else if (qobject_cast<QCheckBox*>(editor))
        {
            connect(editor, SIGNAL(stateChanged(int)), proxy, SLOT(editorDataCommited()));
        }
        else if (qobject_cast<QPlainTextEdit*>(editor))
        {
            connect(editor, SIGNAL(textChanged()), proxy, SLOT(editorDataCommited()));
        }
        else if (qobject_cast<QComboBox*>(editor))
        {
            connect(editor, SIGNAL(currentIndexChanged (int)), proxy, SLOT(editorDataCommited()));
        }
        else if (qobject_cast<QAbstractSpinBox*>(editor) || qobject_cast<QLineEdit*>(editor))
        {
            connect(editor, SIGNAL(editingFinished()), proxy, SLOT(editorDataCommited()));
        }
        else // throw an exception because this is a coding error
            throw std::logic_error ("Dialogue editor type missing");

        connect(proxy, SIGNAL(editorDataCommited(QWidget*, const QModelIndex&, CSMWorld::ColumnBase::Display)),
                this, SLOT(editorDataCommited(QWidget*, const QModelIndex&, CSMWorld::ColumnBase::Display)));

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

CSVWorld::EditWidget::~EditWidget()
{
    for (unsigned i = 0; i < mNestedModels.size(); ++i)
    {
        delete mNestedModels[i];
    }
    delete mNestedTableDispatcher;
}

CSVWorld::EditWidget::EditWidget(QWidget *parent,
        int row, CSMWorld::IdTable* table, CSMWorld::CommandDispatcher& commandDispatcher,
        CSMDoc::Document& document, bool createAndDelete) :
mDispatcher(this, table, commandDispatcher, document),
mNestedTableDispatcher(NULL),
QScrollArea(parent),
mWidgetMapper(NULL),
mNestedTableMapper(NULL),
mMainWidget(NULL),
mCommandDispatcher (commandDispatcher),
mDocument (document),
mTable(table)
{
    remake (row);

    connect(&mDispatcher, SIGNAL(tableMimeDataDropped(QWidget*, const QModelIndex&, const CSMWorld::UniversalId&, const CSMDoc::Document*)),
            this, SIGNAL(tableMimeDataDropped(QWidget*, const QModelIndex&, const CSMWorld::UniversalId&, const CSMDoc::Document*)));
}

void CSVWorld::EditWidget::remake(int row)
{
    for (unsigned i = 0; i < mNestedModels.size(); ++i)
    {
        delete mNestedModels[i];
    }
    mNestedModels.clear();
    delete mNestedTableDispatcher;

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
    if (mNestedTableMapper)
    {
        delete mNestedTableMapper;
        mNestedTableMapper = 0;
    }
    mWidgetMapper = new QDataWidgetMapper (this);

    mWidgetMapper->setModel(mTable);
    mWidgetMapper->setItemDelegate(&mDispatcher);

    QFrame* line = new QFrame(mMainWidget);
    line->setObjectName(QString::fromUtf8("line"));
    line->setGeometry(QRect(320, 150, 118, 3));
    line->setFrameShape(QFrame::HLine);
    line->setFrameShadow(QFrame::Sunken);

    QFrame* line2 = new QFrame(mMainWidget);
    line2->setObjectName(QString::fromUtf8("line"));
    line2->setGeometry(QRect(320, 150, 118, 3));
    line2->setFrameShape(QFrame::HLine);
    line2->setFrameShadow(QFrame::Sunken);

    QVBoxLayout *mainLayout = new QVBoxLayout(mMainWidget);
    QGridLayout *lockedLayout = new QGridLayout();
    QGridLayout *unlockedLayout = new QGridLayout();
    QVBoxLayout *tablesLayout = new QVBoxLayout();

    mainLayout->addLayout(lockedLayout, QSizePolicy::Fixed);
    mainLayout->addWidget(line, 1);
    mainLayout->addLayout(unlockedLayout, QSizePolicy::Preferred);
    mainLayout->addWidget(line2, 1);
    mainLayout->addLayout(tablesLayout, QSizePolicy::Preferred);
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

            if (mTable->hasChildren(mTable->index(row, i)) &&
                    !(flags & CSMWorld::ColumnBase::Flag_Dialogue_List))
            {
                mNestedModels.push_back(new CSMWorld::NestedTableProxyModel (
                            mTable->index(row, i), display, dynamic_cast<CSMWorld::IdTree*>(mTable)));

                int idColumn = mTable->findColumnIndex (CSMWorld::Columns::ColumnId_Id);
                int typeColumn = mTable->findColumnIndex (CSMWorld::Columns::ColumnId_RecordType);

                CSMWorld::UniversalId id = CSMWorld::UniversalId(
                    static_cast<CSMWorld::UniversalId::Type> (mTable->data (mTable->index (row, typeColumn)).toInt()),
                    mTable->data (mTable->index (row, idColumn)).toString().toUtf8().constData());

                NestedTable* table = new NestedTable(mDocument, id, mNestedModels.back(), this);
                // FIXME: does not work well when enum delegates are used
                //table->resizeColumnsToContents();
                table->setEditTriggers(QAbstractItemView::SelectedClicked | QAbstractItemView::CurrentChanged);

                int rows = mTable->rowCount(mTable->index(row, i));
                int rowHeight = (rows == 0) ? table->horizontalHeader()->height() : table->rowHeight(0);
                int tableMaxHeight = (5 * rowHeight)
                    + table->horizontalHeader()->height() + 2 * table->frameWidth();
                table->setMinimumHeight(tableMaxHeight);

                QLabel* label =
                    new QLabel (mTable->headerData (i, Qt::Horizontal, Qt::DisplayRole).toString(), mMainWidget);

                label->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);

                tablesLayout->addWidget(label);
                tablesLayout->addWidget(table);
            }
            else if (!(flags & CSMWorld::ColumnBase::Flag_Dialogue_List))
            {
                mDispatcher.makeDelegate (display);
                QWidget* editor = mDispatcher.makeEditor (display, (mTable->index (row, i)));

                if (editor)
                {
                    mWidgetMapper->addMapping (editor, i);

                    QLabel* label = new QLabel (mTable->headerData (i, Qt::Horizontal).toString(), mMainWidget);

                    label->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
                    editor->setSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

                    if (! (mTable->flags (mTable->index (row, i)) & Qt::ItemIsEditable))
                    {
                        lockedLayout->addWidget (label, locked, 0);
                        lockedLayout->addWidget (editor, locked, 1);
                        ++locked;
                    }
                    else
                    {
                        unlockedLayout->addWidget (label, unlocked, 0);
                        unlockedLayout->addWidget (editor, unlocked, 1);
                        ++unlocked;
                    }
                }
            }
            else
            {
                mNestedModels.push_back(new CSMWorld::NestedTableProxyModel (
                            static_cast<CSMWorld::IdTree *>(mTable)->index(row, i),
                            display, static_cast<CSMWorld::IdTree *>(mTable)));
                mNestedTableMapper = new QDataWidgetMapper (this);

                mNestedTableMapper->setModel(mNestedModels.back());
                // FIXME: lack MIME support?
                mNestedTableDispatcher =
                        new DialogueDelegateDispatcher (this, mTable, mCommandDispatcher, mDocument, mNestedModels.back());
                mNestedTableMapper->setItemDelegate(mNestedTableDispatcher);

                int columnCount =
                    mTable->columnCount(mTable->getModelIndex (mNestedModels.back()->getParentId(), i));
                for (int col = 0; col < columnCount; ++col)
                {
                    int displayRole = mNestedModels.back()->headerData (col,
                            Qt::Horizontal, CSMWorld::ColumnBase::Role_Display).toInt();

                    CSMWorld::ColumnBase::Display display =
                        static_cast<CSMWorld::ColumnBase::Display> (displayRole);

                   mNestedTableDispatcher->makeDelegate (display);

                    // FIXME: assumed all columns are editable
                    QWidget* editor =
                        mNestedTableDispatcher->makeEditor (display, mNestedModels.back()->index (0, col));
                    if (editor)
                    {
                        mNestedTableMapper->addMapping (editor, col);

                        std::string disString = mNestedModels.back()->headerData (col,
                                    Qt::Horizontal, Qt::DisplayRole).toString().toStdString();
                        // Need ot use Qt::DisplayRole in order to get the  correct string
                        // from CSMWorld::Columns
                        QLabel* label = new QLabel (mNestedModels.back()->headerData (col,
                                    Qt::Horizontal, Qt::DisplayRole).toString(), mMainWidget);

                        label->setSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed);
                        editor->setSizePolicy (QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

                        unlockedLayout->addWidget (label, unlocked, 0);
                        unlockedLayout->addWidget (editor, unlocked, 1);
                        ++unlocked;
                    }
                }
                mNestedTableMapper->setCurrentModelIndex(mNestedModels.back()->index(0, 0));
            }
        }
    }

    mWidgetMapper->setCurrentModelIndex(mTable->index(row, 0));

    if (unlocked == 0)
        mainLayout->removeWidget(line);

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
    mLocked(false),
    mDocument(document),
    mCommandDispatcher (document, CSMWorld::UniversalId::getParentType (id.getType()))
{
    connect(mTable, SIGNAL(dataChanged (const QModelIndex&, const QModelIndex&)), this, SLOT(dataChanged(const QModelIndex&)));

    changeCurrentId(id.getId());

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
    connect(revertButton, SIGNAL(clicked()), &mCommandDispatcher, SLOT(executeRevert()));
    connect(deleteButton, SIGNAL(clicked()), &mCommandDispatcher, SLOT(executeDelete()));

    mMainLayout = new QVBoxLayout(mainWidget);

    mEditWidget = new EditWidget(mainWidget,
            mTable->getModelIndex(mCurrentId, 0).row(), mTable, mCommandDispatcher, document, false);
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
        cloneButton->setDisabled (true);
        addButton->setDisabled (true);
        deleteButton->setDisabled (true);
    }

    dataChanged(mTable->getModelIndex (mCurrentId, 0));
    mMainLayout->addLayout (buttonsLayout);
    setWidget (mainWidget);
}

void CSVWorld::DialogueSubView::prevId ()
{
    int newRow = mTable->getModelIndex(mCurrentId, 0).row() - 1;

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

                changeCurrentId(std::string(mTable->data (mTable->index (newRow, 0)).toString().toUtf8().constData()));

                mEditWidget->setDisabled(mLocked);

                return;
        }
        --newRow;
    }
}

void CSVWorld::DialogueSubView::nextId ()
{
    int newRow = mTable->getModelIndex(mCurrentId, 0).row() + 1;

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
        if (!(state == CSMWorld::RecordBase::State_Deleted))
        {
                mEditWidget->remake(newRow);

                setUniversalId(CSMWorld::UniversalId (static_cast<CSMWorld::UniversalId::Type> (mTable->data (mTable->index (newRow, 2)).toInt()),
                                                      mTable->data (mTable->index (newRow, 0)).toString().toUtf8().constData()));

                changeCurrentId(std::string(mTable->data (mTable->index (newRow, 0)).toString().toUtf8().constData()));

                mEditWidget->setDisabled(mLocked);

                return;
        }
        ++newRow;
    }
}

void CSVWorld::DialogueSubView::setEditLock (bool locked)
{
    mLocked = locked;
    QModelIndex currentIndex(mTable->getModelIndex(mCurrentId, 0));

    if (currentIndex.isValid())
    {
        CSMWorld::RecordBase::State state = static_cast<CSMWorld::RecordBase::State>(mTable->data (mTable->index (currentIndex.row(), 1)).toInt());

        mEditWidget->setDisabled (state==CSMWorld::RecordBase::State_Deleted || locked);

        mCommandDispatcher.setEditLock (locked);
    }

}

void CSVWorld::DialogueSubView::dataChanged (const QModelIndex & index)
{
    QModelIndex currentIndex(mTable->getModelIndex(mCurrentId, 0));

    if (currentIndex.isValid() && index.row() == currentIndex.row())
    {
        CSMWorld::RecordBase::State state = static_cast<CSMWorld::RecordBase::State>(mTable->data (mTable->index (currentIndex.row(), 1)).toInt());

        mEditWidget->setDisabled (state==CSMWorld::RecordBase::State_Deleted || mLocked);
    }
}

void CSVWorld::DialogueSubView::tableMimeDataDropped (QWidget* editor,
                                                      const QModelIndex& index,
                                                      const CSMWorld::UniversalId& id,
                                                      const CSMDoc::Document* document)
{
    if (document == &mDocument)
    {
        qobject_cast<DropLineEdit*>(editor)->setText(id.getId().c_str());
    }
}

void CSVWorld::DialogueSubView::requestFocus (const std::string& id)
{
    changeCurrentId(id);

    mEditWidget->remake(mTable->getModelIndex (id, 0).row());
}

void CSVWorld::DialogueSubView::cloneRequest ()
{
    mBottom->cloneRequest(mCurrentId, static_cast<CSMWorld::UniversalId::Type>(mTable->data(mTable->getModelIndex(mCurrentId, 2)).toInt()));
}

void CSVWorld::DialogueSubView::showPreview ()
{
    QModelIndex currentIndex(mTable->getModelIndex(mCurrentId, 0));

    if (currentIndex.isValid() &&
        mTable->getFeatures() & CSMWorld::IdTable::Feature_Preview &&
        currentIndex.row() < mTable->rowCount())
    {
        emit focusId(CSMWorld::UniversalId(CSMWorld::UniversalId::Type_Preview, mCurrentId), "");
    }
}

void CSVWorld::DialogueSubView::viewRecord ()
{
    QModelIndex currentIndex(mTable->getModelIndex (mCurrentId, 0));

    if (currentIndex.isValid() &&
        currentIndex.row() < mTable->rowCount())
    {
        std::pair<CSMWorld::UniversalId, std::string> params = mTable->view (currentIndex.row());

        if (params.first.getType()!=CSMWorld::UniversalId::Type_None)
            emit focusId (params.first, params.second);
    }
}

void CSVWorld::DialogueSubView::changeCurrentId (const std::string& newId)
{
    std::vector<std::string> selection;
    mCurrentId = std::string(newId);

    selection.push_back(mCurrentId);
    mCommandDispatcher.setSelection(selection);
}
