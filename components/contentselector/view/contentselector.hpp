#ifndef CONTENTSELECTOR_HPP
#define CONTENTSELECTOR_HPP

#include <QDialog>

#include "ui_datafilespage.h"

namespace ContentSelectorModel { class ContentModel; }

class QSortFilterProxyModel;

namespace CSVDoc
{
    class FileWidget;
    class AdjusterWidget;
}
namespace ContentSelectorView
{
    enum SelectorFlags
    {
        Flag_Content    = 0x01,     // gamefile combobox & addon list view
        Flag_NewAddon   = 0x02,     // enable project button box (Create/Cancel) and file/adjuster widgets
        Flag_LoadAddon  = 0x04,     // enable project button box (Open/Cancel)
        Flag_Profile    = 0x08      // enable profile combo box
    };

    class ContentSelector : public QWidget
    {
        Q_OBJECT

        unsigned char mFlags;

        static ContentSelector *mInstance;
        static QStringList mFilePaths;

        CSVDoc::FileWidget *mFileWidget;
        CSVDoc::AdjusterWidget *mAdjusterWidget;

    protected:

        ContentSelectorModel::ContentModel *mContentModel;
        QSortFilterProxyModel *mGameFileProxyModel;
        QSortFilterProxyModel *mAddonProxyModel;

    public:

        static void configure(QWidget *subject, unsigned char flags = Flag_Content);
        static ContentSelector &instance();
        static void addFiles(const QString &path);

        void setCheckState(QModelIndex index, QSortFilterProxyModel *model);
        QStringList checkedItemsPaths();
        QString filename() const;
        QStringList selectedFiles() const;

   private:

        explicit ContentSelector(QWidget *parent = 0, unsigned char flags = Flag_Content);
        Ui::DataFilesPage ui;

        void buildContentModel();
        void buildGameFileView();
        void buildAddonView();
        void buildProfilesView();
        void buildNewAddonView();
        void buildLoadAddonView();

        bool isFlagged(SelectorFlags flag) const;

    signals:
        void accepted();
        void rejected();

        void signalProfileChanged(int index);
        void signalGameFileChanged(int value);

        void signalCreateButtonClicked();

    private slots:

        void slotCurrentProfileIndexChanged(int index);
        void slotCurrentGameFileIndexChanged(int index);
        void slotAddonTableItemClicked(const QModelIndex &index);

        void slotUpdateCreateButton (bool);
        void slotUpdateOpenButton(const QStringList &items);
    };
}

#endif // CONTENTSELECTOR_HPP
