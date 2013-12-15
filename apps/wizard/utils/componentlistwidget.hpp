#ifndef COMPONENTLISTWIDGET_HPP
#define COMPONENTLISTWIDGET_HPP

#include <QListWidget>

class ComponentListWidget : public QListWidget
{
    Q_OBJECT

    Q_PROPERTY(QStringList mCheckedItems READ checkedItems)

public:
    ComponentListWidget(QWidget *parent = 0);

    QStringList mCheckedItems;
    QStringList checkedItems();

    void addItem(QListWidgetItem *item);

signals:
    void checkedItemsChanged(const QStringList &items);

private slots:
    void updateCheckedItems(QListWidgetItem *item);
};



#endif // COMPONENTLISTWIDGET_HPP
