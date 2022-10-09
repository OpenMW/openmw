#include "componentlistwidget.hpp"

ComponentListWidget::ComponentListWidget(QWidget* parent)
    : QListWidget(parent)
{
    mCheckedItems = QStringList();

    connect(this, &ComponentListWidget::itemChanged, this,
        qOverload<QListWidgetItem*>(&ComponentListWidget::updateCheckedItems));

    connect(model(), &QAbstractItemModel::rowsInserted, this,
        qOverload<const QModelIndex&, int, int>(&ComponentListWidget::updateCheckedItems));
}

QStringList ComponentListWidget::checkedItems()
{
    mCheckedItems.removeDuplicates();
    return mCheckedItems;
}

void ComponentListWidget::updateCheckedItems(const QModelIndex& index, int start, int end)
{
    updateCheckedItems(item(start));
}

void ComponentListWidget::updateCheckedItems(QListWidgetItem* item)
{
    if (!item)
        return;

    QString text = item->text();

    if (item->checkState() == Qt::Checked)
    {
        if (!mCheckedItems.contains(text))
            mCheckedItems.append(text);
    }
    else
    {
        if (mCheckedItems.contains(text))
            mCheckedItems.removeAll(text);
    }

    mCheckedItems.removeDuplicates();

    emit checkedItemsChanged(mCheckedItems);
}
