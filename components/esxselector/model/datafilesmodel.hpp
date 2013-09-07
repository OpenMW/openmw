#ifndef DATAFILESMODEL_HPP
#define DATAFILESMODEL_HPP

#include <QAbstractTableModel>
#include <QStringList>
#include <QString>
#include <QHash>

namespace EsxModel
{
    class EsmFile;

    typedef QList<const EsmFile *> EsmFileList;

    class DataFilesModel : public QAbstractTableModel
    {
        Q_OBJECT

    public:
        explicit DataFilesModel(QObject *parent = 0);
        virtual ~DataFilesModel();
        virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
        virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;
        bool removeRows(int row, int count, const QModelIndex &parent);
        bool insertRows(int row, int count, const QModelIndex &parent);

        bool moveRow(int oldrow, int row, const QModelIndex &parent = QModelIndex());

        virtual Qt::ItemFlags flags(const QModelIndex &index) const;

        virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
        virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

        virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
        void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

        inline QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const
           {
            QModelIndex idx = QAbstractTableModel::index(row, 0, parent);
            return idx;
        }

        void setEncoding(const QString &encoding);

        void addFiles(const QString &path);

        void uncheckAll();

        Qt::DropActions supportedDropActions() const;
        QStringList mimeTypes() const;
        QMimeData *mimeData(const QModelIndexList &indexes) const;
        bool dropMimeData(const QMimeData *data, Qt::DropAction action, int row, int column, const QModelIndex &parent);

        EsmFileList checkedItems();
        EsmFileList uncheckedItems();
        QStringList checkedItemsPaths();

        Qt::CheckState checkState(const QModelIndex &index);
        void setCheckState(const QModelIndex &index, Qt::CheckState state);

        QModelIndex indexFromItem(const EsmFile *item) const;
        const EsmFile* findItem(const QString &name);
        const EsmFile* item(int row) const;

    signals:
        void checkedItemsChanged(const EsmFileList &items);

    private:

        bool canBeChecked(const EsmFile *file) const;
        void addFile(const EsmFile *file);

        EsmFileList mFiles;
        QHash<QString, Qt::CheckState> mCheckStates;

        QString mEncoding;

    };
}
#endif // DATAFILESMODEL_HPP
