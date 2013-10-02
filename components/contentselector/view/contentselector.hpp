#ifndef CONTENTSELECTOR_HPP
#define CONTENTSELECTOR_HPP

#include <QDialog>

#include "ui_datafilespage.h"
#include "../model/contentmodel.hpp"

class QSortFilterProxyModel;
class TextInputDialog;

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

        TextInputDialog *mNewDialog;

    protected:

        ContentSelectorModel::ContentModel *mContentModel;
        QSortFilterProxyModel *mGameFileProxyModel;
        QSortFilterProxyModel *mAddonProxyModel;

    public:
        explicit ContentSelector(QWidget *parent = 0, unsigned char flags = Flag_Content);

        static void configure(QWidget *subject, unsigned char flags = Flag_Content);
        static ContentSelector &instance();
        static void addFiles(const QString &path);

        void setCheckStates (const QStringList &list);
        QStringList checkedItemsPaths();
        ContentSelectorModel::ContentFileList *CheckedItems();

        QString filename() const;
        ContentSelectorModel::ContentFileList selectedFiles() const;
        QStringList selectedFilePaths() const;

        void addProfile (const QString &item, bool setAsCurrent = false);
        void removeProfile (const QString &item);
        int getProfileIndex (const QString &item) const;
        void setProfileIndex (int index);
        QString getProfileText() const;

   private:

        Ui::DataFilesPage ui;

        void buildContentModel();
        void buildGameFileView();
        void buildAddonView();
        void buildProfilesView();
        void buildNewAddonView();
        void buildLoadAddonView();

        bool isFlagged(SelectorFlags flag) const;
        QString getNewProfileName();
        void enableProfilesComboBox();

    signals:
        void accepted();
        void rejected();

        void signalProfileChanged(int index);
        void signalGameFileChanged(int value);

        void signalCreateButtonClicked();

        void signalProfileRenamed(QString,QString);
        void signalProfileChanged(QString,QString);
        void signalProfileDeleted(QString);
        void signalProfileAdded();

    private slots:

        void slotProfileTextChanged (const QString &text);
        void slotCurrentProfileIndexChanged(int index);
        void slotCurrentGameFileIndexChanged(int index);
        void slotAddonTableItemClicked(const QModelIndex &index);

        void slotUpdateCreateButton (bool);
       // void slotUpdateOpenButton(const QStringList &items);

        // Action slots
        void on_newProfileAction_triggered();
        void on_deleteProfileAction_triggered();
    };
}

#endif // CONTENTSELECTOR_HPP
