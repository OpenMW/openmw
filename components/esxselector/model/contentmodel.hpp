#ifndef CONTENTMODEL_HPP
#define CONTENTMODEL_HPP

#include <QAbstractTableModel>

namespace EsxModel
{
    class EsmFile;

    typedef QList<EsmFile *> ContentFileList;

    class ContentModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        explicit ContentModel(QObject *parent = 0);

        void setEncoding(const QString &encoding);

        int rowCount(const QModelIndex &parent = QModelIndex()) const;
        int columnCount(const QModelIndex &parent = QModelIndex()) const;

        QVariant data(const QModelIndex &index, int role) const;
        Qt::ItemFlags flags(const QModelIndex &index) const;
        bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);

        bool insertRows(int position, int rows, const QModelIndex &index = QModelIndex());
        bool removeRows(int position, int rows, const QModelIndex &index = QModelIndex());

        Qt::DropActions supportedDropActions() const;
        QStringList mimeTypes() const;
        QMimeData *mimeData(const QModelIndexList &indexes) const;
        bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

        void addFiles(const QString &path);

        QModelIndex indexFromItem(EsmFile *item) const;
        EsxModel::EsmFile *findItem(const QString &name);

        Qt::CheckState checkState(const QModelIndex &index);
        void setCheckState(const QModelIndex &index, Qt::CheckState state);
        ContentFileList checkedItems() const;
        void uncheckAll();
/*
        QModelIndex index(int row, int column, const QModelIndex &parent) const;
        QModelIndex parent(const QModelIndex &child) const;
*/
    private:

        void addFile(EsmFile *file);
        EsmFile* item(int row) const;
        bool canBeChecked(const EsmFile *file) const;

        ContentFileList mFiles;
        QHash<QString, Qt::CheckState> mCheckStates;
        QString mEncoding;

    signals:

    public slots:

    };
}
#endif // CONTENTMODEL_HPP
