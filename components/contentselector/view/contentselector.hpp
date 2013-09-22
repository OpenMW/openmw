#ifndef CONTENTSELECTOR_HPP
#define CONTENTSELECTOR_HPP

#include <QDialog>

#include "ui_datafilespage.h"

namespace ContentSelectorModel { class ContentModel; }

class QSortFilterProxyModel;

namespace ContentSelectorView
{
    class ContentSelector : public QDialog, protected Ui::DataFilesPage
    {
        Q_OBJECT

    protected:

        ContentSelectorModel::ContentModel *mContentModel;
        QSortFilterProxyModel *mGameFileProxyModel;
        QSortFilterProxyModel *mAddonProxyModel;

    public:

        explicit ContentSelector(QWidget *parent = 0);

        static ContentSelector &cast(QWidget *subject);      //static constructor function for singleton performance.

        void addFiles(const QString &path);
        void setCheckState(QModelIndex index, QSortFilterProxyModel *model);
        QStringList checkedItemsPaths();

   private:

        void buildContentModel();
        void buildGameFileView();
        void buildAddonView();
        void buildProfilesView();

    signals:
        void profileChanged(int index);

    private slots:
        void updateViews();
        void slotCurrentProfileIndexChanged(int index);
        void slotCurrentGameFileIndexChanged(int index);
        void slotAddonTableItemClicked(const QModelIndex &index);
    };
}

#endif // CONTENTSELECTOR_HPP
