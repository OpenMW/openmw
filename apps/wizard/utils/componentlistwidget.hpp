#ifndef COMPONENTLISTWIDGET_HPP
#define COMPONENTLISTWIDGET_HPP

#include <QListWidget>

class ComponentListWidget : public QListWidget
{
    Q_OBJECT

    Q_PROPERTY(QStringList mCheckedItems READ checkedItems)

public:
    ComponentListWidget(QWidget *parent = nullptr);

    QStringList mCheckedItems;
    QStringList checkedItems();

signals:
    void checkedItemsChanged(const QStringList &items);

private slots:
    void updateCheckedItems(QListWidgetItem *item);
    void updateCheckedItems(const QModelIndex &index, int start, int end);
};

#endif // COMPONENTLISTWIDGET_HPP
