#ifndef CONTENTMODEL_HPP
#define CONTENTMODEL_HPP

#include "loadordererror.hpp"
#include <QAbstractTableModel>
#include <QIcon>
#include <QSet>
#include <QStringList>

#include <set>

namespace ContentSelectorModel
{
    class EsmFile;

    typedef QList<EsmFile*> ContentFileList;

    enum ContentType
    {
        ContentType_GameFile,
        ContentType_Addon
    };

    class ContentModel : public QAbstractTableModel
    {
        Q_OBJECT
    public:
        explicit ContentModel(QObject* parent, QIcon& warningIcon, QIcon& errorIcon, bool showOMWScripts);
        ~ContentModel();

        void setEncoding(const QString& encoding);

        int rowCount(const QModelIndex& parent = QModelIndex()) const override;
        int columnCount(const QModelIndex& parent = QModelIndex()) const override;

        QVariant data(const QModelIndex& index, int role) const override;
        Qt::ItemFlags flags(const QModelIndex& index) const override;
        bool setData(const QModelIndex& index, const QVariant& value, int role = Qt::EditRole) override;

        bool insertRows(int position, int rows, const QModelIndex& index = QModelIndex()) override;
        bool removeRows(int position, int rows, const QModelIndex& index = QModelIndex()) override;

        Qt::DropActions supportedDropActions() const override;
        QStringList mimeTypes() const override;
        QMimeData* mimeData(const QModelIndexList& indexes) const override;
        bool dropMimeData(
            const QMimeData* data, Qt::DropAction action, int row, int column, const QModelIndex& parent) override;

        void addFiles(const QString& path, bool newfiles);
        void sortFiles();
        bool containsDataFiles(const QString& path);
        void clearFiles();

        QModelIndex indexFromItem(const EsmFile* item) const;
        const EsmFile* item(const QString& name) const;
        const EsmFile* item(int row) const;
        EsmFile* item(int row);
        QStringList gameFiles() const;
        void setCurrentGameFile(const EsmFile* file);

        bool isEnabled(const QModelIndex& index) const;
        bool setCheckState(const EsmFile* file, bool isChecked);
        bool isNew(const QString& filepath) const;
        void setNew(const EsmFile* file, bool isChecked);
        void setNonUserContent(const QStringList& fileList);
        void setContentList(const QStringList& fileList);
        ContentFileList checkedItems() const;
        void uncheckAll();

        void refreshModel(std::initializer_list<int> roles = {});

    private:
        void addFile(EsmFile* file);

        /// Checks a specific plug-in for load order errors
        /// \return all errors found for specific plug-in
        QList<LoadOrderError> checkForLoadOrderErrors(const EsmFile* file, int row) const;

        ///  \return true if plug-in has a Load Order error
        bool isLoadOrderError(const EsmFile* file) const;

        QString toolTip(const EsmFile* file) const;

        const EsmFile* mGameFile;
        ContentFileList mFiles;
        QStringList mNonUserContent;
        std::set<const EsmFile*> mCheckedFiles;
        QHash<QString, bool> mNewFiles;
        QString mEncoding;
        QIcon mWarningIcon;
        QIcon mErrorIcon;
        bool mShowOMWScripts;

        QString mErrorToolTips[ContentSelectorModel::LoadOrderError::ErrorCode_LoadOrder]
            = { tr("Unable to find dependent file: %1"), tr("Dependent file needs to be active: %1"),
                  tr("This file needs to load after %1") };

    public:
        QString mMimeType;
        QStringList mMimeTypes;
        int mColumnCount;
        Qt::DropActions mDropActions;
    };
}
#endif // CONTENTMODEL_HPP
