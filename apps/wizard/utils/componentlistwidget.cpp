#include "componentlistwidget.hpp"

#include <QDebug>
#include <QStringList>

ComponentListWidget::ComponentListWidget(QWidget *parent) :
    QListWidget(parent)
{
    mCheckedItems = QStringList();

    connect(this, SIGNAL(itemChanged(QListWidgetItem *)),
            this, SLOT(updateCheckedItems(QListWidgetItem *)));
}

void ComponentListWidget::addItem(QListWidgetItem *item)
{
    // The model does not emit a dataChanged signal when items are added
    // So we need to update manually
    QListWidget::insertItem(count(), item);
    updateCheckedItems(item);

}

QStringList ComponentListWidget::checkedItems()
{
    mCheckedItems.removeDuplicates();
    return mCheckedItems;
}

void ComponentListWidget::updateCheckedItems(QListWidgetItem *item)
{
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
