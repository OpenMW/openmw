#include "componentlistwidget.hpp"

#include <QDebug>

ComponentListWidget::ComponentListWidget(QWidget *parent) :
    QListWidget(parent)
{
    mCheckedItems = QStringList();

    connect(this, SIGNAL(itemChanged(QListWidgetItem *)),
            this, SLOT(updateCheckedItems(QListWidgetItem *)));

    connect(model(), SIGNAL(rowsInserted(QModelIndex, int, int)),
            this, SLOT(updateCheckedItems(QModelIndex, int, int)));
}

QStringList ComponentListWidget::checkedItems()
{
    mCheckedItems.removeDuplicates();
    return mCheckedItems;
}

void ComponentListWidget::updateCheckedItems(const QModelIndex &index, int start, int end)
{
    updateCheckedItems(item(start));
}

void ComponentListWidget::updateCheckedItems(QListWidgetItem *item)
{
    if (!item)
        return;

    QString text = item->text();

    if (item->checkState() == Qt::Checked) {
        if (!mCheckedItems.contains(text))
            mCheckedItems.append(text);
    } else {
        if (mCheckedItems.contains(text))
            mCheckedItems.removeAll(text);
    }

    mCheckedItems.removeDuplicates();

    emit checkedItemsChanged(mCheckedItems);

}
