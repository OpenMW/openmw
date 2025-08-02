#include "dialoguesubview.hpp"

#include <algorithm>
#include <memory>
#include <stdexcept>
#include <type_traits>
#include <utility>

#include <QAbstractItemModel>
#include <QCheckBox>
#include <QComboBox>
#include <QDataWidgetMapper>
#include <QFrame>
#include <QGridLayout>
#include <QHeaderView>
#include <QLabel>
#include <QLineEdit>
#include <QMenu>
#include <QModelIndex>
#include <QPlainTextEdit>
#include <QRect>
#include <QScrollBar>
#include <QSize>
#include <QSizePolicy>
#include <QVBoxLayout>
#include <QVariant>
#include <QWidget>

#include <apps/opencs/model/prefs/category.hpp>
#include <apps/opencs/model/prefs/setting.hpp>
#include <apps/opencs/model/world/commanddispatcher.hpp>
#include <apps/opencs/model/world/data.hpp>
#include <apps/opencs/model/world/disabletag.hpp>
#include <apps/opencs/model/world/idtablebase.hpp>
#include <apps/opencs/model/world/universalid.hpp>
#include <apps/opencs/view/doc/subview.hpp>

#include "../../model/doc/document.hpp"
#include "../../model/world/columnbase.hpp"
#include "../../model/world/columns.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/idtree.hpp"
#include "../../model/world/nestedtableproxymodel.hpp"
#include "../../model/world/record.hpp"
#include "../../model/world/tablemimedata.hpp"

#include "../../model/prefs/state.hpp"

#include "../widget/coloreditor.hpp"
#include "../widget/droplineedit.hpp"

#include "nestedtable.hpp"
#include "recordbuttonbar.hpp"
#include "tablebottombox.hpp"
#include "util.hpp"

class QPainter;
class QPoint;

/*
==============================NotEditableSubDelegate==========================================
*/
CSVWorld::NotEditableSubDelegate::NotEditableSubDelegate(const CSMWorld::IdTable* table, QObject* parent)
    : QAbstractItemDelegate(parent)
    , mTable(table)
{
}

void CSVWorld::NotEditableSubDelegate::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    QLabel* label = qobject_cast<QLabel*>(editor);
    if (!label)
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

    CSMWorld::Columns::ColumnId columnId
        = static_cast<CSMWorld::Columns::ColumnId>(mTable->getColumnId(index.column()));

    if (QMetaType::QString == v.typeId())
    {
        label->setText(v.toString());
    }
    else if (CSMWorld::Columns::hasEnums(columnId))
    {
        int data = v.toInt();
        std::vector<std::pair<int, std::string>> enumNames(CSMWorld::Columns::getEnums(columnId));

        label->setText(QString::fromUtf8(enumNames.at(data).second.c_str()));
    }
    else
    {
        label->setText(v.toString());
    }
}

void CSVWorld::NotEditableSubDelegate::setModelData(
    QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    // not editable widgets will not save model data
}

void CSVWorld::NotEditableSubDelegate::paint(
    QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // does nothing
}

QSize CSVWorld::NotEditableSubDelegate::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return QSize();
}

QWidget* CSVWorld::NotEditableSubDelegate::createEditor(
    QWidget* parent, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    QLabel* label = new QLabel(parent);
    label->setTextInteractionFlags(Qt::TextSelectableByMouse);
    return label;
}

/*
==============================DialogueDelegateDispatcherProxy==========================================
*/
CSVWorld::DialogueDelegateDispatcherProxy::DialogueDelegateDispatcherProxy(
    QWidget* editor, CSMWorld::ColumnBase::Display display)
    : mEditor(editor)
    , mDisplay(display)
{
}

void CSVWorld::DialogueDelegateDispatcherProxy::editorDataCommited()
{
    if (mIndex.has_value())
    {
        emit editorDataCommited(mEditor, mIndex.value(), mDisplay);
    }
}

void CSVWorld::DialogueDelegateDispatcherProxy::setIndex(const QModelIndex& index)
{
    mIndex = index;
}

QWidget* CSVWorld::DialogueDelegateDispatcherProxy::getEditor() const
{
    return mEditor;
}

/*
==============================DialogueDelegateDispatcher==========================================
*/

CSVWorld::DialogueDelegateDispatcher::DialogueDelegateDispatcher(QObject* parent, CSMWorld::IdTable* table,
    CSMWorld::CommandDispatcher& commandDispatcher, CSMDoc::Document& document, QAbstractItemModel* model)
    : mParent(parent)
    , mTable(model ? model : table)
    , mCommandDispatcher(commandDispatcher)
    , mDocument(document)
    , mNotEditableDelegate(table, parent)
{
}

CSVWorld::CommandDelegate* CSVWorld::DialogueDelegateDispatcher::makeDelegate(CSMWorld::ColumnBase::Display display)
{
    CommandDelegate* delegate = nullptr;
    std::map<int, CommandDelegate*>::const_iterator delegateIt(mDelegates.find(display));
    if (delegateIt == mDelegates.end())
    {
        delegate
            = CommandDelegateFactoryCollection::get().makeDelegate(display, &mCommandDispatcher, mDocument, mParent);
        mDelegates.insert(std::make_pair(display, delegate));
    }
    else
    {
        delegate = delegateIt->second;
    }
    return delegate;
}

void CSVWorld::DialogueDelegateDispatcher::editorDataCommited(
    QWidget* editor, const QModelIndex& index, CSMWorld::ColumnBase::Display display)
{
    setModelData(editor, mTable, index, display);
}

void CSVWorld::DialogueDelegateDispatcher::setEditorData(QWidget* editor, const QModelIndex& index) const
{
    CSMWorld::ColumnBase::Display display = CSMWorld::ColumnBase::Display_None;
    if (index.parent().isValid())
    {
        display
            = static_cast<CSMWorld::ColumnBase::Display>(static_cast<CSMWorld::IdTree*>(mTable)
                                                             ->nestedHeaderData(index.parent().column(), index.column(),
                                                                 Qt::Horizontal, CSMWorld::ColumnBase::Role_Display)
                                                             .toInt());
    }
    else
    {
        display = static_cast<CSMWorld::ColumnBase::Display>(
            mTable->headerData(index.column(), Qt::Horizontal, CSMWorld::ColumnBase::Role_Display).toInt());
    }

    QLabel* label = qobject_cast<QLabel*>(editor);
    if (label)
    {
        mNotEditableDelegate.setEditorData(label, index);
        return;
    }

    std::map<int, CommandDelegate*>::const_iterator delegateIt(mDelegates.find(display));
    if (delegateIt != mDelegates.end())
    {
        delegateIt->second->setEditorData(editor, index, true);
    }

    for (const auto& proxy : mProxys)
    {
        if (proxy->getEditor() == editor)
        {
            proxy->setIndex(index);
        }
    }
}

void CSVWorld::DialogueDelegateDispatcher::setModelData(
    QWidget* editor, QAbstractItemModel* model, const QModelIndex& index) const
{
    setModelData(editor, model, index, CSMWorld::ColumnBase::Display_None);
}

void CSVWorld::DialogueDelegateDispatcher::setModelData(
    QWidget* editor, QAbstractItemModel* model, const QModelIndex& index, CSMWorld::ColumnBase::Display display) const
{
    std::map<int, CommandDelegate*>::const_iterator delegateIt(mDelegates.find(display));
    if (delegateIt != mDelegates.end())
    {
        delegateIt->second->setModelData(editor, model, index);
    }
}

void CSVWorld::DialogueDelegateDispatcher::paint(
    QPainter* painter, const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    // Does nothing
}

QSize CSVWorld::DialogueDelegateDispatcher::sizeHint(const QStyleOptionViewItem& option, const QModelIndex& index) const
{
    return QSize(); // silencing warning, otherwise does nothing
}

QWidget* CSVWorld::DialogueDelegateDispatcher::makeEditor(
    CSMWorld::ColumnBase::Display display, const QModelIndex& index)
{
    QVariant variant = index.data();
    if (!variant.isValid())
    {
        variant = index.data(Qt::DisplayRole);
        if (!variant.isValid())
        {
            return nullptr;
        }
    }

    QWidget* editor = nullptr;
    if (!(mTable->flags(index) & Qt::ItemIsEditable))
    {
        return mNotEditableDelegate.createEditor(qobject_cast<QWidget*>(mParent), QStyleOptionViewItem(), index);
    }

    std::map<int, CommandDelegate*>::iterator delegateIt(mDelegates.find(display));

    if (delegateIt != mDelegates.end())
    {
        editor
            = delegateIt->second->createEditor(qobject_cast<QWidget*>(mParent), QStyleOptionViewItem(), index, display);

        DialogueDelegateDispatcherProxy* proxy = new DialogueDelegateDispatcherProxy(editor, display);

        // NOTE: For each entry in CSVWorld::CommandDelegate::createEditor() a corresponding entry
        // is required here
        if (qobject_cast<CSVWidget::DropLineEdit*>(editor))
        {
            connect(static_cast<CSVWidget::DropLineEdit*>(editor), &CSVWidget::DropLineEdit::editingFinished, proxy,
                qOverload<>(&DialogueDelegateDispatcherProxy::editorDataCommited));

            connect(static_cast<CSVWidget::DropLineEdit*>(editor), &CSVWidget::DropLineEdit::tableMimeDataDropped,
                proxy, qOverload<>(&DialogueDelegateDispatcherProxy::editorDataCommited));
        }
        else if (qobject_cast<QCheckBox*>(editor))
        {
            connect(static_cast<QCheckBox*>(editor), &QCheckBox::stateChanged, proxy,
                qOverload<>(&DialogueDelegateDispatcherProxy::editorDataCommited));
        }
        else if (qobject_cast<QPlainTextEdit*>(editor))
        {
            connect(static_cast<QPlainTextEdit*>(editor), &QPlainTextEdit::textChanged, proxy,
                qOverload<>(&DialogueDelegateDispatcherProxy::editorDataCommited));
        }
        else if (qobject_cast<QComboBox*>(editor))
        {
            connect(static_cast<QComboBox*>(editor), qOverload<int>(&QComboBox::currentIndexChanged), proxy,
                qOverload<>(&DialogueDelegateDispatcherProxy::editorDataCommited));
        }
        else if (qobject_cast<QAbstractSpinBox*>(editor) || qobject_cast<QLineEdit*>(editor))
        {
            connect(static_cast<QAbstractSpinBox*>(editor), &QAbstractSpinBox::editingFinished, proxy,
                qOverload<>(&DialogueDelegateDispatcherProxy::editorDataCommited));
        }
        else if (qobject_cast<CSVWidget::ColorEditor*>(editor))
        {
            connect(static_cast<CSVWidget::ColorEditor*>(editor), &CSVWidget::ColorEditor::pickingFinished, proxy,
                qOverload<>(&DialogueDelegateDispatcherProxy::editorDataCommited));
        }
        else // throw an exception because this is a coding error
            throw std::logic_error("Dialogue editor type missing");

        connect(proxy,
            qOverload<QWidget*, const QModelIndex&, CSMWorld::ColumnBase::Display>(
                &DialogueDelegateDispatcherProxy::editorDataCommited),
            this, &DialogueDelegateDispatcher::editorDataCommited);

        mProxys.push_back(proxy); // deleted in the destructor
    }
    return editor;
}

CSVWorld::DialogueDelegateDispatcher::~DialogueDelegateDispatcher()
{
    for (auto* proxy : mProxys)
    {
        delete proxy; // unique_ptr could be handy
    }
}

CSVWorld::IdContextMenu::IdContextMenu(QWidget* widget, CSMWorld::ColumnBase::Display display)
    : QObject(widget)
    , mWidget(widget)
    , mIdType(CSMWorld::TableMimeData::convertEnums(display))
{
    Q_ASSERT(mWidget != nullptr);
    Q_ASSERT(CSMWorld::ColumnBase::isId(display));
    Q_ASSERT(mIdType != CSMWorld::UniversalId::Type_None);

    mWidget->setContextMenuPolicy(Qt::CustomContextMenu);
    connect(mWidget, &QWidget::customContextMenuRequested, this, &IdContextMenu::showContextMenu);

    mEditIdAction = new QAction(this);
    connect(mEditIdAction, &QAction::triggered, this, qOverload<>(&IdContextMenu::editIdRequest));

    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(mWidget);
    if (lineEdit != nullptr)
    {
        mContextMenu = lineEdit->createStandardContextMenu();
    }
    else
    {
        mContextMenu = new QMenu(mWidget);
    }
}

void CSVWorld::IdContextMenu::excludeId(const std::string& id)
{
    mExcludedIds.insert(id);
}

QString CSVWorld::IdContextMenu::getWidgetValue() const
{
    QLineEdit* lineEdit = qobject_cast<QLineEdit*>(mWidget);
    QLabel* label = qobject_cast<QLabel*>(mWidget);

    QString value = "";
    if (lineEdit != nullptr)
    {
        value = lineEdit->text();
    }
    else if (label != nullptr)
    {
        value = label->text();
    }
    return value;
}

void CSVWorld::IdContextMenu::addEditIdActionToMenu(const QString& text)
{
    mEditIdAction->setText(text);
    if (mContextMenu->actions().isEmpty())
    {
        mContextMenu->addAction(mEditIdAction);
    }
    else if (mContextMenu->actions().first() != mEditIdAction)
    {
        QAction* action = mContextMenu->actions().first();
        mContextMenu->insertAction(action, mEditIdAction);
        mContextMenu->insertSeparator(action);
    }
}

void CSVWorld::IdContextMenu::removeEditIdActionFromMenu()
{
    if (mContextMenu->actions().isEmpty())
    {
        return;
    }

    if (mContextMenu->actions().first() == mEditIdAction)
    {
        mContextMenu->removeAction(mEditIdAction);
        if (!mContextMenu->actions().isEmpty() && mContextMenu->actions().first()->isSeparator())
        {
            mContextMenu->removeAction(mContextMenu->actions().first());
        }
    }
}

void CSVWorld::IdContextMenu::showContextMenu(const QPoint& pos)
{
    QString value = getWidgetValue();
    bool isExcludedId = mExcludedIds.find(value.toUtf8().constData()) != mExcludedIds.end();
    if (!value.isEmpty() && !isExcludedId)
    {
        addEditIdActionToMenu("Edit '" + value + "'");
    }
    else
    {
        removeEditIdActionFromMenu();
    }

    if (!mContextMenu->actions().isEmpty())
    {
        mContextMenu->exec(mWidget->mapToGlobal(pos));
    }
}

void CSVWorld::IdContextMenu::editIdRequest()
{
    CSMWorld::UniversalId editId(mIdType, getWidgetValue().toUtf8().constData());
    emit editIdRequest(editId, "");
}

/*
=============================================================EditWidget=====================================================
*/

void CSVWorld::EditWidget::createEditorContextMenu(
    QWidget* editor, CSMWorld::ColumnBase::Display display, int currentRow) const
{
    Q_ASSERT(editor != nullptr);

    if (CSMWorld::ColumnBase::isId(display)
        && CSMWorld::TableMimeData::convertEnums(display) != CSMWorld::UniversalId::Type_None)
    {
        int idColumn = mTable->findColumnIndex(CSMWorld::Columns::ColumnId_Id);
        QString id = mTable->data(mTable->index(currentRow, idColumn)).toString();

        IdContextMenu* menu = new IdContextMenu(editor, display);
        // Current ID is already opened, so no need to create Edit 'ID' action for it
        menu->excludeId(id.toUtf8().constData());
        connect(menu, qOverload<const CSMWorld::UniversalId&, const std::string&>(&IdContextMenu::editIdRequest), this,
            &EditWidget::editIdRequest);
    }
}

CSVWorld::EditWidget::~EditWidget()
{
    for (auto* model : mNestedModels)
        delete model;

    if (mDispatcher)
        delete mDispatcher;

    if (mNestedTableDispatcher)
        delete mNestedTableDispatcher;
}

CSVWorld::EditWidget::EditWidget(QWidget* parent, int row, CSMWorld::IdTable* table,
    CSMWorld::CommandDispatcher& commandDispatcher, CSMDoc::Document& document, bool createAndDelete)
    : QScrollArea(parent)
    , mWidgetMapper(nullptr)
    , mNestedTableMapper(nullptr)
    , mDispatcher(nullptr)
    , mNestedTableDispatcher(nullptr)
    , mMainWidget(nullptr)
    , mTable(table)
    , mCommandDispatcher(commandDispatcher)
    , mDocument(document)
{
    remake(row);
}

void CSVWorld::EditWidget::remake(int row)
{
    if (mMainWidget)
    {
        QWidget* del = this->takeWidget();
        del->deleteLater();
    }
    mMainWidget = new QWidget(this);

    for (auto* model : mNestedModels)
        delete model;

    mNestedModels.clear();

    if (mDispatcher)
        delete mDispatcher;
    mDispatcher = new DialogueDelegateDispatcher(nullptr /*this*/, mTable, mCommandDispatcher, mDocument);

    if (mNestedTableDispatcher)
        delete mNestedTableDispatcher;

    // not sure if widget mapper can handle deleting the widgets that were mapped
    if (mWidgetMapper)
        delete mWidgetMapper;

    mWidgetMapper = new QDataWidgetMapper(this);
    mWidgetMapper->setModel(mTable);
    mWidgetMapper->setItemDelegate(mDispatcher);

    if (mNestedTableMapper)
        delete mNestedTableMapper;

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

    QVBoxLayout* mainLayout = new QVBoxLayout(mMainWidget);
    QGridLayout* lockedLayout = new QGridLayout();
    QGridLayout* unlockedLayout = new QGridLayout();
    QVBoxLayout* tablesLayout = new QVBoxLayout();

    mainLayout->addLayout(lockedLayout, QSizePolicy::Fixed);
    mainLayout->addWidget(line, 1);
    mainLayout->addLayout(unlockedLayout, QSizePolicy::Preferred);
    mainLayout->addWidget(line2, 1);
    mainLayout->addLayout(tablesLayout, QSizePolicy::Preferred);
    mainLayout->addStretch(1);

    int blockedColumn = mTable->searchColumnIndex(CSMWorld::Columns::ColumnId_Blocked);
    bool isBlocked = mTable->data(mTable->index(row, blockedColumn)).toInt();

    int unlocked = 0;
    int locked = 0;
    const int columns = mTable->columnCount();

    for (int i = 0; i < columns; ++i)
    {
        int flags = mTable->headerData(i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Flags).toInt();

        if (flags & CSMWorld::ColumnBase::Flag_Dialogue)
        {
            CSMWorld::ColumnBase::Display display = static_cast<CSMWorld::ColumnBase::Display>(
                mTable->headerData(i, Qt::Horizontal, CSMWorld::ColumnBase::Role_Display).toInt());

            if (mTable->hasChildren(mTable->index(row, i)) && !(flags & CSMWorld::ColumnBase::Flag_Dialogue_List))
            {
                CSMWorld::IdTree* innerTable = &dynamic_cast<CSMWorld::IdTree&>(*mTable);
                mNestedModels.push_back(
                    new CSMWorld::NestedTableProxyModel(mTable->index(row, i), display, innerTable));

                int idColumn = mTable->findColumnIndex(CSMWorld::Columns::ColumnId_Id);
                int typeColumn = mTable->findColumnIndex(CSMWorld::Columns::ColumnId_RecordType);

                CSMWorld::UniversalId id = CSMWorld::UniversalId(
                    static_cast<CSMWorld::UniversalId::Type>(mTable->data(mTable->index(row, typeColumn)).toInt()),
                    mTable->data(mTable->index(row, idColumn)).toString().toUtf8().constData());

                bool editable = true;
                bool fixedRows = false;
                QVariant v = mTable->index(row, i).data();
                if (v.canConvert<CSMWorld::ColumnBase::TableEditModes>())
                {
                    assert(QString(v.typeName()) == "CSMWorld::ColumnBase::TableEditModes");

                    if (v.value<CSMWorld::ColumnBase::TableEditModes>() == CSMWorld::ColumnBase::TableEdit_None)
                        editable = false;
                    else if (v.value<CSMWorld::ColumnBase::TableEditModes>()
                        == CSMWorld::ColumnBase::TableEdit_FixedRows)
                        fixedRows = true;
                }

                // Create and display nested table only if it's editable.
                if (editable)
                {
                    NestedTable* table
                        = new NestedTable(mDocument, id, mNestedModels.back(), this, editable, fixedRows);
                    table->resizeColumnsToContents();
                    if (isBlocked)
                        table->setEditTriggers(QAbstractItemView::NoEditTriggers);

                    int rows = mTable->rowCount(mTable->index(row, i));
                    int rowHeight = (rows == 0) ? table->horizontalHeader()->height() : table->rowHeight(0);
                    int headerHeight = table->horizontalHeader()->height();
                    int tableMaxHeight = (5 * rowHeight) + headerHeight + (2 * table->frameWidth());
                    table->setMinimumHeight(tableMaxHeight);

                    QString headerText = mTable->headerData(i, Qt::Horizontal, Qt::DisplayRole).toString();
                    QLabel* label = new QLabel(headerText, mMainWidget);
                    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);

                    tablesLayout->addWidget(label);
                    tablesLayout->addWidget(table);

                    connect(table, &NestedTable::editRequest, this, &EditWidget::editIdRequest);
                }
            }
            else if (!(flags & CSMWorld::ColumnBase::Flag_Dialogue_List))
            {
                mDispatcher->makeDelegate(display);
                QWidget* editor = mDispatcher->makeEditor(display, (mTable->index(row, i)));

                if (editor)
                {
                    mWidgetMapper->addMapping(editor, i);

                    QLabel* label = new QLabel(mTable->headerData(i, Qt::Horizontal).toString(), mMainWidget);

                    label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                    editor->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

                    // HACK: the blocked checkbox needs to keep the same position
                    // FIXME: unfortunately blocked record displays a little differently to unblocked one
                    if (!(mTable->flags(mTable->index(row, i)) & Qt::ItemIsEditable) || i == blockedColumn)
                    {
                        lockedLayout->addWidget(label, locked, 0);
                        lockedLayout->addWidget(editor, locked, 1);
                        ++locked;
                    }
                    else
                    {
                        unlockedLayout->addWidget(label, unlocked, 0);
                        unlockedLayout->addWidget(editor, unlocked, 1);
                        ++unlocked;
                    }

                    if (CSMWorld::DisableTag::isDisableTag(mTable->index(row, i).data()))
                    {
                        editor->setEnabled(false);
                        label->setEnabled(false);
                    }

                    createEditorContextMenu(editor, display, row);
                }
            }
            else // Flag_Dialogue_List
            {
                CSMWorld::IdTree* tree = static_cast<CSMWorld::IdTree*>(mTable);
                mNestedTableMapper = new QDataWidgetMapper(this);

                mNestedTableMapper->setModel(tree);
                // FIXME: lack MIME support?
                mNestedTableDispatcher
                    = new DialogueDelegateDispatcher(nullptr /*this*/, mTable, mCommandDispatcher, mDocument, tree);
                mNestedTableMapper->setRootIndex(tree->index(row, i));
                mNestedTableMapper->setItemDelegate(mNestedTableDispatcher);

                int columnCount = tree->columnCount(tree->index(row, i));
                for (int col = 0; col < columnCount; ++col)
                {
                    int displayRole
                        = tree->nestedHeaderData(i, col, Qt::Horizontal, CSMWorld::ColumnBase::Role_Display).toInt();

                    display = static_cast<CSMWorld::ColumnBase::Display>(displayRole);

                    mNestedTableDispatcher->makeDelegate(display);

                    // FIXME: assumed all columns are editable
                    QWidget* editor
                        = mNestedTableDispatcher->makeEditor(display, tree->index(0, col, tree->index(row, i)));
                    if (editor)
                    {
                        mNestedTableMapper->addMapping(editor, col);

                        // Need to use Qt::DisplayRole in order to get the  correct string
                        // from CSMWorld::Columns
                        QLabel* label = new QLabel(
                            tree->nestedHeaderData(i, col, Qt::Horizontal, Qt::DisplayRole).toString(), mMainWidget);

                        label->setSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed);
                        editor->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::Fixed);

                        unlockedLayout->addWidget(label, unlocked, 0);
                        unlockedLayout->addWidget(editor, unlocked, 1);
                        ++unlocked;

                        if (CSMWorld::DisableTag::isDisableTag(tree->index(0, col, tree->index(row, i)).data()))
                        {
                            editor->setEnabled(false);
                            label->setEnabled(false);
                        }

                        if (!isBlocked)
                            createEditorContextMenu(editor, display, row);
                        else
                            editor->setEnabled(false);
                    }
                }
                mNestedTableMapper->setCurrentModelIndex(tree->index(0, 0, tree->index(row, i)));
            }
        }
    }

    mWidgetMapper->setCurrentModelIndex(mTable->index(row, 0));

    if (unlocked == 0)
        mainLayout->removeWidget(line);

    this->setWidget(mMainWidget);
    this->setWidgetResizable(true);
}

QVBoxLayout& CSVWorld::SimpleDialogueSubView::getMainLayout()
{
    return *mMainLayout;
}

CSMWorld::IdTable& CSVWorld::SimpleDialogueSubView::getTable()
{
    return *mTable;
}

CSMWorld::CommandDispatcher& CSVWorld::SimpleDialogueSubView::getCommandDispatcher()
{
    return mCommandDispatcher;
}

CSVWorld::EditWidget& CSVWorld::SimpleDialogueSubView::getEditWidget()
{
    return *mEditWidget;
}

bool CSVWorld::SimpleDialogueSubView::isLocked() const
{
    return mLocked;
}

CSVWorld::SimpleDialogueSubView::SimpleDialogueSubView(const CSMWorld::UniversalId& id, CSMDoc::Document& document)
    : SubView(id)
    , mEditWidget(nullptr)
    , mMainLayout(nullptr)
    , mTable(dynamic_cast<CSMWorld::IdTable*>(document.getData().getTableModel(id)))
    , mLocked(false)
    , mDocument(document)
    , mCommandDispatcher(document, CSMWorld::UniversalId::getParentType(id.getType()))
{
    connect(mTable, &CSMWorld::IdTable::dataChanged, this, &SimpleDialogueSubView::dataChanged);
    connect(mTable, &CSMWorld::IdTable::rowsAboutToBeRemoved, this, &SimpleDialogueSubView::rowsAboutToBeRemoved);

    updateCurrentId();

    QWidget* mainWidget = new QWidget(this);

    mMainLayout = new QVBoxLayout(mainWidget);
    setWidget(mainWidget);

    int idColumn = getTable().findColumnIndex(CSMWorld::Columns::ColumnId_Id);

    mEditWidget = new EditWidget(mainWidget, mTable->getModelIndex(getUniversalId().getId(), idColumn).row(), mTable,
        mCommandDispatcher, document, false);

    mMainLayout->addWidget(mEditWidget);
    mEditWidget->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);

    dataChanged(mTable->getModelIndex(getUniversalId().getId(), idColumn));

    connect(mEditWidget, &EditWidget::editIdRequest, this, &SimpleDialogueSubView::focusId);
}

void CSVWorld::SimpleDialogueSubView::setEditLock(bool locked)
{
    if (!mEditWidget) // hack to indicate that getUniversalId().getId() is no longer valid
        return;

    int idColumn = getTable().findColumnIndex(CSMWorld::Columns::ColumnId_Id);
    mLocked = locked;
    QModelIndex currentIndex(mTable->getModelIndex(getUniversalId().getId(), idColumn));

    if (currentIndex.isValid())
    {
        CSMWorld::RecordBase::State state
            = static_cast<CSMWorld::RecordBase::State>(mTable->data(mTable->index(currentIndex.row(), 1)).toInt());

        mEditWidget->setDisabled(state == CSMWorld::RecordBase::State_Deleted || locked);

        mCommandDispatcher.setEditLock(locked);
    }
}

void CSVWorld::SimpleDialogueSubView::dataChanged(const QModelIndex& index)
{
    int idColumn = getTable().findColumnIndex(CSMWorld::Columns::ColumnId_Id);
    QModelIndex currentIndex(mTable->getModelIndex(getUniversalId().getId(), idColumn));

    if (currentIndex.isValid() && (index.parent().isValid() ? index.parent().row() : index.row()) == currentIndex.row())
    {
        CSMWorld::RecordBase::State state
            = static_cast<CSMWorld::RecordBase::State>(mTable->data(mTable->index(currentIndex.row(), 1)).toInt());

        mEditWidget->setDisabled(state == CSMWorld::RecordBase::State_Deleted || mLocked);

        // Check if the changed data should force refresh (rebuild) the dialogue subview
        int flags = 0;
        if (index.parent().isValid()) // TODO: check that index is topLeft
        {
            flags = static_cast<CSMWorld::IdTree*>(mTable)
                        ->nestedHeaderData(
                            index.parent().column(), index.column(), Qt::Horizontal, CSMWorld::ColumnBase::Role_Flags)
                        .toInt();
        }
        else
        {
            flags = mTable->headerData(index.column(), Qt::Horizontal, CSMWorld::ColumnBase::Role_Flags).toInt();
        }

        if (flags & CSMWorld::ColumnBase::Flag_Dialogue_Refresh)
        {
            int y = mEditWidget->verticalScrollBar()->value();
            mEditWidget->remake(index.parent().isValid() ? index.parent().row() : index.row());
            mEditWidget->verticalScrollBar()->setValue(y);
        }
    }
}

void CSVWorld::SimpleDialogueSubView::rowsAboutToBeRemoved(const QModelIndex& parent, int start, int end)
{
    int idColumn = getTable().findColumnIndex(CSMWorld::Columns::ColumnId_Id);
    QModelIndex currentIndex(mTable->getModelIndex(getUniversalId().getId(), idColumn));

    if (!currentIndex.isValid())
    {
        return;
    }

    if (currentIndex.parent() == parent && currentIndex.row() >= start && currentIndex.row() <= end)
    {
        if (mEditWidget)
        {
            delete mEditWidget;
            mEditWidget = nullptr;
        }
        emit closeRequest(this);
    }
}

void CSVWorld::SimpleDialogueSubView::updateCurrentId()
{
    std::vector<std::string> selection;
    selection.push_back(getUniversalId().getId());
    mCommandDispatcher.setSelection(selection);
}

void CSVWorld::DialogueSubView::addButtonBar()
{
    if (mButtons)
        return;

    mButtons = new RecordButtonBar(getUniversalId(), getTable(), mBottom, &getCommandDispatcher(), this);

    getMainLayout().insertWidget(1, mButtons);

    // connections
    connect(mButtons, &RecordButtonBar::showPreview, this, &DialogueSubView::showPreview);
    connect(mButtons, &RecordButtonBar::viewRecord, this, &DialogueSubView::viewRecord);
    connect(mButtons, &RecordButtonBar::switchToRow, this, &DialogueSubView::switchToRow);

    connect(this, &DialogueSubView::universalIdChanged, mButtons, &RecordButtonBar::universalIdChanged);
}

CSVWorld::DialogueSubView::DialogueSubView(
    const CSMWorld::UniversalId& id, CSMDoc::Document& document, const CreatorFactoryBase& creatorFactory, bool sorting)
    : SimpleDialogueSubView(id, document)
    , mButtons(nullptr)
{
    // bottom box
    mBottom = new TableBottomBox(creatorFactory, document, id, this);

    connect(mBottom, &TableBottomBox::requestFocus, this, &DialogueSubView::requestFocus);

    // layout
    getMainLayout().addWidget(mBottom);

    connect(&CSMPrefs::State::get(), &CSMPrefs::State::settingChanged, this, &DialogueSubView::settingChanged);
    CSMPrefs::get()["ID Dialogues"].update();
}

void CSVWorld::DialogueSubView::setEditLock(bool locked)
{
    SimpleDialogueSubView::setEditLock(locked);

    if (mButtons)
        mButtons->setEditLock(locked);
}

void CSVWorld::DialogueSubView::settingChanged(const CSMPrefs::Setting* setting)
{
    if (*setting == "ID Dialogues/toolbar")
    {
        if (setting->isTrue())
        {
            addButtonBar();
        }
        else if (mButtons)
        {
            getMainLayout().removeWidget(mButtons);
            delete mButtons;
            mButtons = nullptr;
        }
    }
}

void CSVWorld::DialogueSubView::showPreview()
{
    int idColumn = getTable().findColumnIndex(CSMWorld::Columns::ColumnId_Id);
    QModelIndex currentIndex(getTable().getModelIndex(getUniversalId().getId(), idColumn));

    if (currentIndex.isValid() && getTable().getFeatures() & CSMWorld::IdTable::Feature_Preview
        && currentIndex.row() < getTable().rowCount())
    {
        emit focusId(CSMWorld::UniversalId(CSMWorld::UniversalId::Type_Preview, getUniversalId().getId()), "");
    }
}

void CSVWorld::DialogueSubView::viewRecord()
{
    int idColumn = getTable().findColumnIndex(CSMWorld::Columns::ColumnId_Id);
    QModelIndex currentIndex(getTable().getModelIndex(getUniversalId().getId(), idColumn));

    if (currentIndex.isValid() && currentIndex.row() < getTable().rowCount())
    {
        std::pair<CSMWorld::UniversalId, std::string> params = getTable().view(currentIndex.row());

        if (params.first.getType() != CSMWorld::UniversalId::Type_None)
            emit focusId(params.first, params.second);
    }
}

void CSVWorld::DialogueSubView::switchToRow(int row)
{
    int idColumn = getTable().findColumnIndex(CSMWorld::Columns::ColumnId_Id);
    std::string id = getTable().data(getTable().index(row, idColumn)).toString().toUtf8().constData();

    int typeColumn = getTable().findColumnIndex(CSMWorld::Columns::ColumnId_RecordType);
    CSMWorld::UniversalId::Type type
        = static_cast<CSMWorld::UniversalId::Type>(getTable().data(getTable().index(row, typeColumn)).toInt());

    setUniversalId(CSMWorld::UniversalId(type, id));
    updateCurrentId();

    getEditWidget().remake(row);

    int stateColumn = getTable().findColumnIndex(CSMWorld::Columns::ColumnId_Modification);
    CSMWorld::RecordBase::State state
        = static_cast<CSMWorld::RecordBase::State>(getTable().data(getTable().index(row, stateColumn)).toInt());

    getEditWidget().setDisabled(isLocked() || state == CSMWorld::RecordBase::State_Deleted);
}

void CSVWorld::DialogueSubView::requestFocus(const std::string& id)
{
    int idColumn = getTable().findColumnIndex(CSMWorld::Columns::ColumnId_Id);
    QModelIndex index = getTable().getModelIndex(id, idColumn);

    if (index.isValid())
        switchToRow(index.row());
}
