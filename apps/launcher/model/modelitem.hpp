#ifndef MODELITEM_HPP
#define MODELITEM_HPP

#include <QObject>
#include <QList>

class ModelItem : public QObject
{
    Q_OBJECT

public:
    ModelItem(ModelItem *parent = 0);
    ~ModelItem();

    ModelItem *parent();
    int row() const;

    int childCount() const;
    int childRow(ModelItem *child) const;
    ModelItem *child(int row);

    void appendChild(ModelItem *child);
    void removeChild(int row);

    //virtual bool acceptChild(ModelItem *child);

protected:
    ModelItem *mParentItem;
    QList<ModelItem*> mChildItems;
};

#endif
