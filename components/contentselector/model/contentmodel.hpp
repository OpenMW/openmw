#ifndef CONTENTMODEL_HPP
#define CONTENTMODEL_HPP

#include <QAbstractTableModel>
#include <QStringList>

namespace ContentSelectorModel
{
    class EsmFile;

    typedef QList<EsmFile *> ContentFileList;

    enum ContentType
    {
        ContentType_GameFile,
        ContentType_Addon
    };

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

        QModelIndex indexFromItem(const EsmFile *item) const;
        const EsmFile *item(const QString &name) const;

        bool isEnabled (QModelIndex index) const;
        bool isChecked(const QString &filepath) const;
        bool setCheckState(const QString &filepath, bool isChecked);
        void setCheckStates (const QStringList &fileList, bool isChecked);
        ContentFileList checkedItems() const;
        void uncheckAll();

        void refreshModel();

    private:

        void addFile(EsmFile *file);
        const EsmFile *item(int row) const;
        EsmFile *item(int row);

        void sortFiles();

        ContentFileList mFiles;
        QHash<QString, Qt::CheckState> mCheckStates;
        QTextCodec *mCodec;
        QString mEncoding;

    public:

        QString mMimeType;
        QStringList mMimeTypes;
        int mColumnCount;
        Qt::ItemFlags mDragDropFlags;
        Qt::DropActions mDropActions;
    };
}
#endif // CONTENTMODEL_HPP
