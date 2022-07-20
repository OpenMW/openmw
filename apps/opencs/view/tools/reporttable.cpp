#include "reporttable.hpp"

#include <algorithm>

#include <QHeaderView>
#include <QAction>
#include <QMenu>
#include <QStyledItemDelegate>
#include <QTextDocument>
#include <QPainter>
#include <QContextMenuEvent>
#include <QMouseEvent>
#include <QSortFilterProxyModel>

#include "../../model/tools/reportmodel.hpp"

#include "../../model/prefs/state.hpp"
#include "../../model/prefs/shortcut.hpp"

#include "../../view/world/idtypedelegate.hpp"

namespace CSVTools
{
    class RichTextDelegate : public QStyledItemDelegate
    {
        public:

            RichTextDelegate (QObject *parent = nullptr);

            void paint(QPainter *painter, const QStyleOptionViewItem& option,
                const QModelIndex& index) const override;
    };
}

CSVTools::RichTextDelegate::RichTextDelegate (QObject *parent) : QStyledItemDelegate (parent)
{}

void CSVTools::RichTextDelegate::paint(QPainter *painter, const QStyleOptionViewItem& option,
    const QModelIndex& index) const
{
    QTextDocument document;
    QVariant value = index.data (Qt::DisplayRole);
    if (value.isValid() && !value.isNull())
    {
        document.setHtml (value.toString());
        painter->translate (option.rect.topLeft());
        document.drawContents (painter);
        painter->translate (-option.rect.topLeft());
    }
}


void CSVTools::ReportTable::contextMenuEvent (QContextMenuEvent *event)
{
    QModelIndexList selectedRows = selectionModel()->selectedRows();

    // create context menu
    QMenu menu (this);

    if (!selectedRows.empty())
    {
        menu.addAction (mShowAction);
        menu.addAction (mRemoveAction);

        bool found = false;
        for (QModelIndexList::const_iterator iter (selectedRows.begin());
            iter!=selectedRows.end(); ++iter)
        {
            QString hint = mProxyModel->data (mProxyModel->index (iter->row(), 2)).toString();

            if (!hint.isEmpty() && hint[0]=='R')
            {
                found = true;
                break;
            }
        }

        if (found)
            menu.addAction (mReplaceAction);
    }

    if (mRefreshAction)
        menu.addAction (mRefreshAction);

    menu.exec (event->globalPos());
}

void CSVTools::ReportTable::mouseMoveEvent (QMouseEvent *event)
{
    if (event->buttons() & Qt::LeftButton)
        startDragFromTable (*this);
}

void CSVTools::ReportTable::mouseDoubleClickEvent (QMouseEvent *event)
{
    Qt::KeyboardModifiers modifiers =
        event->modifiers() & (Qt::ShiftModifier | Qt::ControlModifier);

    QModelIndex index = currentIndex();

    selectionModel()->select (index,
        QItemSelectionModel::Clear | QItemSelectionModel::Select | QItemSelectionModel::Rows);

    std::map<Qt::KeyboardModifiers, DoubleClickAction>::iterator iter =
        mDoubleClickActions.find (modifiers);

    if (iter==mDoubleClickActions.end())
    {
        event->accept();
        return;
    }

    switch (iter->second)
    {
        case Action_None:

            event->accept();
            break;

        case Action_Edit:

            event->accept();
            showSelection();
            break;

        case Action_Remove:

            event->accept();
            removeSelection();
            break;

        case Action_EditAndRemove:

            event->accept();
            showSelection();
            removeSelection();
            break;
    }
}

CSVTools::ReportTable::ReportTable (CSMDoc::Document& document,
    const CSMWorld::UniversalId& id, bool richTextDescription, int refreshState,
    QWidget *parent)
: CSVWorld::DragRecordTable (document, parent), mModel (document.getReport (id)),
  mRefreshAction (nullptr), mRefreshState (refreshState)
{
    horizontalHeader()->setSectionResizeMode (QHeaderView::Interactive);
    horizontalHeader()->setStretchLastSection (true);
    verticalHeader()->hide();
    setSortingEnabled (true);
    setSelectionBehavior (QAbstractItemView::SelectRows);
    setSelectionMode (QAbstractItemView::ExtendedSelection);

    mProxyModel = new QSortFilterProxyModel (this);
    mProxyModel->setSortCaseSensitivity(Qt::CaseInsensitive);
    mProxyModel->setSourceModel (mModel);
    mProxyModel->setSortRole(Qt::UserRole);

    setModel (mProxyModel);
    setColumnHidden (2, true);

    mIdTypeDelegate = CSVWorld::IdTypeDelegateFactory().makeDelegate (nullptr,
        mDocument, this);

    setItemDelegateForColumn (0, mIdTypeDelegate);

    if (richTextDescription)
        setItemDelegateForColumn (mModel->columnCount()-1, new RichTextDelegate (this));

    mShowAction = new QAction (tr ("Show"), this);
    connect (mShowAction, SIGNAL (triggered()), this, SLOT (showSelection()));
    addAction (mShowAction);
    CSMPrefs::Shortcut* showShortcut = new CSMPrefs::Shortcut("reporttable-show", this);
    showShortcut->associateAction(mShowAction);

    mRemoveAction = new QAction (tr ("Remove from list"), this);
    connect (mRemoveAction, SIGNAL (triggered()), this, SLOT (removeSelection()));
    addAction (mRemoveAction);
    CSMPrefs::Shortcut* removeShortcut = new CSMPrefs::Shortcut("reporttable-remove", this);
    removeShortcut->associateAction(mRemoveAction);

    mReplaceAction = new QAction (tr ("Replace"), this);
    connect (mReplaceAction, SIGNAL (triggered()), this, SIGNAL (replaceRequest()));
    addAction (mReplaceAction);
    CSMPrefs::Shortcut* replaceShortcut = new CSMPrefs::Shortcut("reporttable-replace", this);
    replaceShortcut->associateAction(mReplaceAction);

    if (mRefreshState)
    {
        mRefreshAction = new QAction (tr ("Refresh"), this);
        mRefreshAction->setEnabled (!(mDocument.getState() & mRefreshState));
        connect (mRefreshAction, SIGNAL (triggered()), this, SIGNAL (refreshRequest()));
        addAction (mRefreshAction);
        CSMPrefs::Shortcut* refreshShortcut = new CSMPrefs::Shortcut("reporttable-refresh", this);
        refreshShortcut->associateAction(mRefreshAction);
    }

    mDoubleClickActions.insert (std::make_pair (Qt::NoModifier, Action_Edit));
    mDoubleClickActions.insert (std::make_pair (Qt::ShiftModifier, Action_Remove));
    mDoubleClickActions.insert (std::make_pair (Qt::ControlModifier, Action_EditAndRemove));

    connect (&CSMPrefs::State::get(), SIGNAL (settingChanged (const CSMPrefs::Setting *)),
        this, SLOT (settingChanged (const CSMPrefs::Setting *)));
    CSMPrefs::get()["Reports"].update();
}

std::vector<CSMWorld::UniversalId> CSVTools::ReportTable::getDraggedRecords() const
{
    std::vector<CSMWorld::UniversalId> ids;

    QModelIndexList selectedRows = selectionModel()->selectedRows();

    for (QModelIndexList::const_iterator iter (selectedRows.begin()); iter!=selectedRows.end();
        ++iter)
    {
        ids.push_back (mModel->getUniversalId (mProxyModel->mapToSource (*iter).row()));
    }

    return ids;
}

std::vector<int> CSVTools::ReportTable::getReplaceIndices (bool selection) const
{
    std::vector<int> indices;

    if (selection)
    {
        QModelIndexList selectedRows = selectionModel()->selectedRows();

        std::vector<int> rows;

        for (QModelIndexList::const_iterator iter (selectedRows.begin()); iter!=selectedRows.end();
            ++iter)
        {
            rows.push_back (mProxyModel->mapToSource (*iter).row());
        }

        std::sort (rows.begin(), rows.end());

        for (std::vector<int>::const_iterator iter (rows.begin()); iter!=rows.end(); ++iter)
        {
            QString hint = mModel->data (mModel->index (*iter, 2)).toString();

            if (!hint.isEmpty() && hint[0]=='R')
                indices.push_back (*iter);
        }
    }
    else
    {
        for (int i=0; i<mModel->rowCount(); ++i)
        {
            QString hint = mModel->data (mModel->index (i, 2)).toString();

            if (!hint.isEmpty() && hint[0]=='R')
                indices.push_back (i);
        }
    }

    return indices;
}

void CSVTools::ReportTable::flagAsReplaced (int index)
{
    mModel->flagAsReplaced (index);
}

void CSVTools::ReportTable::settingChanged (const CSMPrefs::Setting *setting)
{
    if (setting->getParent()->getKey()=="Reports")
    {
        QString base ("double");
        QString key = setting->getKey().c_str();
        if (key.startsWith (base))
        {
            QString modifierString = key.mid (base.size());
            Qt::KeyboardModifiers modifiers;

            if (modifierString=="-s")
                modifiers = Qt::ShiftModifier;
            else if (modifierString=="-c")
                modifiers = Qt::ControlModifier;
            else if (modifierString=="-sc")
                modifiers = Qt::ShiftModifier | Qt::ControlModifier;

            DoubleClickAction action = Action_None;

            std::string value = setting->toString();

            if (value=="Edit")
                action = Action_Edit;
            else if (value=="Remove")
                action = Action_Remove;
            else if (value=="Edit And Remove")
                action = Action_EditAndRemove;

            mDoubleClickActions[modifiers] = action;

            return;
        }
    }
    else if (*setting=="Records/type-format")
        mIdTypeDelegate->settingChanged (setting);
}

void CSVTools::ReportTable::showSelection()
{
    QModelIndexList selectedRows = selectionModel()->selectedRows();

    for (QModelIndexList::const_iterator iter (selectedRows.begin()); iter!=selectedRows.end();
        ++iter)
    {
        int row = mProxyModel->mapToSource (*iter).row();
        emit editRequest (mModel->getUniversalId (row), mModel->getHint (row));
    }
}

void CSVTools::ReportTable::removeSelection()
{
    QModelIndexList selectedRows = selectionModel()->selectedRows();

    std::vector<int> rows;

    for (QModelIndexList::iterator iter (selectedRows.begin()); iter!=selectedRows.end();
        ++iter)
    {
        rows.push_back (mProxyModel->mapToSource (*iter).row());
    }

    std::sort (rows.begin(), rows.end());

    for (std::vector<int>::const_reverse_iterator iter (rows.rbegin()); iter!=rows.rend(); ++iter)
        mProxyModel->removeRows (*iter, 1);

    selectionModel()->clear();
}

void CSVTools::ReportTable::clear()
{
    mModel->clear();
}

void CSVTools::ReportTable::stateChanged (int state, CSMDoc::Document *document)
{
    if (mRefreshAction)
        mRefreshAction->setEnabled (!(state & mRefreshState));
}
