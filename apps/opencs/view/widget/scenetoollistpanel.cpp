#include "scenetoollistpanel.hpp"

#include <QAbstractItemView>
#include <QApplication>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QMargins>
#include <QModelIndex>
#include <QPoint>
#include <QStyle>
#include <QTableWidget>

CSVWidget::SceneToolListPanel::SceneToolListPanel(int textColumnCount, QWidget* parent)
    : QFrame(parent, Qt::Popup)
    , mTable(new QTableWidget(0, textColumnCount + 1, this))
{
    QHBoxLayout* layout = new QHBoxLayout(this);

    layout->setContentsMargins(QMargins(0, 0, 0, 0));

    mTable->setShowGrid(false);
    mTable->verticalHeader()->hide();
    mTable->horizontalHeader()->hide();
    mTable->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    mTable->horizontalHeader()->setSectionResizeMode(textColumnCount, QHeaderView::ResizeToContents);
    mTable->setSelectionMode(QAbstractItemView::NoSelection);
    mTable->setEditTriggers(QAbstractItemView::NoEditTriggers);

    layout->addWidget(mTable);

    connect(mTable, &QTableWidget::clicked, this, &SceneToolListPanel::clicked);
}

void CSVWidget::SceneToolListPanel::setRows(const std::vector<QStringList>& rows)
{
    const int rowCount = static_cast<int>(rows.size());
    const int removeColumn = mTable->columnCount() - 1;

    mTable->setRowCount(rowCount);

    for (int row = 0; row < rowCount; ++row)
    {
        for (int column = 0; column < removeColumn; ++column)
            mTable->setItem(row, column, new QTableWidgetItem(rows[row].value(column)));

        mTable->setItem(row, removeColumn,
            new QTableWidgetItem(QApplication::style()->standardIcon(QStyle::SP_TitleBarCloseButton), ""));
    }

    if (rows.empty())
        hide();
}

void CSVWidget::SceneToolListPanel::showAt(const QPoint& position)
{
    if (mTable->rowCount() == 0)
        return;

    move(position);
    show();
}

void CSVWidget::SceneToolListPanel::clicked(const QModelIndex& index)
{
    if (index.column() == mTable->columnCount() - 1)
        emit removeRequest(index.row());
    else
    {
        hide();
        emit selectRequest(index.row());
    }
}
