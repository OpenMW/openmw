#ifndef MODELITEM_HPP
#define MODELITEM_HPP

#include <QMimeData>
#include <QList>

namespace ContentSelectorModel
{
    class ModelItem : public QMimeData
    {
        Q_OBJECT

    public:
        ModelItem(ModelItem *parent = nullptr);
        //ModelItem(const ModelItem *parent = 0);

        ~ModelItem();

        ModelItem *parent() const;
        int row() const;

        int childCount() const;
        int childRow(ModelItem *child) const;
        ModelItem *child(int row);

        void appendChild(ModelItem *child);
        void removeChild(int row);

        bool hasFormat(const QString &mimetype) const override;

        //virtual bool acceptChild(ModelItem *child);

    protected:
        ModelItem *mParentItem;
        QList<ModelItem*> mChildItems;
    };
}

#endif
