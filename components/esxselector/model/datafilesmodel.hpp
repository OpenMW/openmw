#ifndef DATAFILESMODEL_HPP
#define DATAFILESMODEL_HPP

#include <QAbstractTableModel>
#include <QStringList>
#include <QString>
#include <QHash>


class EsmFile;

class DataFilesModel : public QAbstractTableModel
{
    Q_OBJECT

public:
    explicit DataFilesModel(QObject *parent = 0);
    virtual ~DataFilesModel();
    virtual int columnCount(const QModelIndex &parent = QModelIndex()) const;
    virtual int rowCount(const QModelIndex &parent = QModelIndex()) const;

    bool moveRow(int oldrow, int row, const QModelIndex &parent = QModelIndex());

    virtual Qt::ItemFlags flags(const QModelIndex &index) const;

    virtual QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const;
    virtual QVariant data(const QModelIndex &index, int role = Qt::DisplayRole) const;

    virtual bool setData(const QModelIndex &index, const QVariant &value, int role = Qt::EditRole);
    void sort(int column, Qt::SortOrder order = Qt::AscendingOrder);

    inline QModelIndex index(int row, int column, const QModelIndex &parent = QModelIndex()) const
       { return QAbstractTableModel::index(row, column, parent); }

    void setEncoding(const QString &encoding);

    void addFiles(const QString &path);

    void uncheckAll();

    QStringList checkedItems();
    QStringList uncheckedItems();
    QStringList checkedItemsPaths();

    Qt::CheckState checkState(const QModelIndex &index);
    void setCheckState(const QModelIndex &index, Qt::CheckState state);

    QModelIndex indexFromItem(EsmFile *item) const;
    EsmFile* findItem(const QString &name);
    EsmFile* item(int row) const;

signals:
    void checkedItemsChanged(const QStringList &items);
    
private:
    bool canBeChecked(EsmFile *file) const;
    void addFile(EsmFile *file);
    
    QList<EsmFile *> mFiles;
    QHash<QString, Qt::CheckState> mCheckStates;

    QString mEncoding;

};

#endif // DATAFILESMODEL_HPP
