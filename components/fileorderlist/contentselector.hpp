#ifndef CONTENTSELECTOR_HPP
#define CONTENTSELECTOR_HPP

#include <QWidget>

#include "ui_datafilespage.h"

class DataFilesModel;
class PluginsProxyModel;
class QSortFilterProxyModel;

namespace FileOrderList
{
    class MasterProxyModel;

    class ContentSelector : public QWidget, protected Ui::DataFilesPage
    {
        Q_OBJECT

    protected:

        DataFilesModel *mDataFilesModel;
        MasterProxyModel *mMasterProxyModel;
        PluginsProxyModel *mPluginsProxyModel;

    public:
        explicit ContentSelector(QWidget *parent = 0);

        void buildModelsAndViews();

        void addFiles(const QString &path);
        void setEncoding(const QString &encoding);
        void setCheckState(QModelIndex index, QSortFilterProxyModel *model);
        QStringList checkedItemsPaths();

    signals:
        void profileChanged(int index);

    private slots:
        void updateViews();
        void slotCurrentProfileIndexChanged(int index);
        void slotCurrentMasterIndexChanged(int index);
    };
}

#endif // CONTENTSELECTOR_HPP
