#ifndef CONTENTMODEL_HPP
#define CONTENTMODEL_HPP

#include <QAbstractTableModel>
#include <QStringList>
#include <QSet>
#include <QIcon>
#include "loadordererror.hpp"

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
        explicit ContentModel(QObject *parent, QIcon warningIcon);
        ~ContentModel();

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
        void clearFiles();

        QModelIndex indexFromItem(const EsmFile *item) const;
        const EsmFile *item(const QString &name) const;
        QStringList gameFiles() const;

        bool isEnabled (QModelIndex index) const;
        bool isChecked(const QString &filepath) const;
        bool setCheckState(const QString &filepath, bool isChecked);
        void setContentList(const QStringList &fileList);
        ContentFileList checkedItems() const;
        void uncheckAll();

        void refreshModel();

        /// Checks all plug-ins for load order errors and updates mPluginsWithLoadOrderError with plug-ins with issues
        void checkForLoadOrderErrors();

    private:

        void addFile(EsmFile *file);
        const EsmFile *item(int row) const;
        EsmFile *item(int row);

        void sortFiles();

        /// Checks a specific plug-in for load order errors
        /// \return all errors found for specific plug-in
        QList<LoadOrderError> checkForLoadOrderErrors(const EsmFile *file, int row) const;

        ///  \return true if plug-in has a Load Order error
        bool isLoadOrderError(const EsmFile *file) const;

        QString toolTip(const EsmFile *file) const;

        ContentFileList mFiles;
        QHash<QString, Qt::CheckState> mCheckStates;
        QSet<QString> mPluginsWithLoadOrderError;
        QString mEncoding;
        QIcon mWarningIcon;

    public:

        QString mMimeType;
        QStringList mMimeTypes;
        int mColumnCount;
        Qt::DropActions mDropActions;
    };
}
#endif // CONTENTMODEL_HPP
