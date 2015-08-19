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

#include "../../view/world/idtypedelegate.hpp"

namespace CSVTools
{
    class RichTextDelegate : public QStyledItemDelegate
    {
        public:

            RichTextDelegate (QObject *parent = 0);

            virtual void paint(QPainter *painter, const QStyleOptionViewItem& option,
                const QModelIndex& index) const;
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
  mRefreshAction (0), mRefreshState (refreshState)
{
#if QT_VERSION >= QT_VERSION_CHECK(5,0,0)
    horizontalHeader()->setSectionResizeMode (QHeaderView::Interactive);
#else
    horizontalHeader()->setResizeMode (QHeaderView::Interactive);
#endif
    horizontalHeader()->setStretchLastSection (true);
    verticalHeader()->hide();
    setSortingEnabled (true);
    setSelectionBehavior (QAbstractItemView::SelectRows);
    setSelectionMode (QAbstractItemView::ExtendedSelection);

    mProxyModel = new QSortFilterProxyModel (this);
    mProxyModel->setSourceModel (mModel);

    setModel (mProxyModel);
    setColumnHidden (2, true);

    mIdTypeDelegate = CSVWorld::IdTypeDelegateFactory().makeDelegate (0,
        mDocument, this);

    setItemDelegateForColumn (0, mIdTypeDelegate);

    if (richTextDescription)
        setItemDelegateForColumn (mModel->columnCount()-1, new RichTextDelegate (this));

    mShowAction = new QAction (tr ("Show"), this);
    connect (mShowAction, SIGNAL (triggered()), this, SLOT (showSelection()));
    addAction (mShowAction);

    mRemoveAction = new QAction (tr ("Remove from list"), this);
    connect (mRemoveAction, SIGNAL (triggered()), this, SLOT (removeSelection()));
    addAction (mRemoveAction);

    mReplaceAction = new QAction (tr ("Replace"), this);
    connect (mReplaceAction, SIGNAL (triggered()), this, SIGNAL (replaceRequest()));
    addAction (mReplaceAction);

    if (mRefreshState)
    {
        mRefreshAction = new QAction (tr ("Refresh"), this);
        mRefreshAction->setEnabled (!(mDocument.getState() & mRefreshState));
        connect (mRefreshAction, SIGNAL (triggered()), this, SIGNAL (refreshRequest()));
        addAction (mRefreshAction);
    }

    mDoubleClickActions.insert (std::make_pair (Qt::NoModifier, Action_Edit));
    mDoubleClickActions.insert (std::make_pair (Qt::ShiftModifier, Action_Remove));
    mDoubleClickActions.insert (std::make_pair (Qt::ControlModifier, Action_EditAndRemove));
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

void CSVTools::ReportTable::updateUserSetting (const QString& name, const QStringList& list)
{
    mIdTypeDelegate->updateUserSetting (name, list);

    QString base ("report-input/double");
    if (name.startsWith (base))
    {
        QString modifierString = name.mid (base.size());
        Qt::KeyboardModifiers modifiers = 0;

        if (modifierString=="-s")
            modifiers = Qt::ShiftModifier;
        else if (modifierString=="-c")
            modifiers = Qt::ControlModifier;
        else if (modifierString=="-sc")
            modifiers = Qt::ShiftModifier | Qt::ControlModifier;

        DoubleClickAction action = Action_None;

        QString value = list.at (0);

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
