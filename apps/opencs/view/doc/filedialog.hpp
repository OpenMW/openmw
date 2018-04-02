#ifndef FILEDIALOG_HPP
#define FILEDIALOG_HPP

#include <QDialog>
#include <QModelIndex>

#include <experimental/filesystem>
#include "adjusterwidget.hpp"

#ifndef CS_QT_BOOST_FILESYSTEM_PATH_DECLARED
#define CS_QT_BOOST_FILESYSTEM_PATH_DECLARED
Q_DECLARE_METATYPE (sfs::path)
#endif

#include "ui_filedialog.h"

namespace ContentSelectorView
{
    class ContentSelector;
}

namespace CSVDoc
{
    class FileWidget;

    class FileDialog : public QDialog
    {
        Q_OBJECT

    private:

        ContentSelectorView::ContentSelector *mSelector;
        Ui::FileDialog ui;
        ContentAction mAction;
        FileWidget *mFileWidget;
        AdjusterWidget *mAdjusterWidget;
        bool mDialogBuilt;

    public:

        explicit FileDialog(QWidget *parent = 0);
        void showDialog (ContentAction action);

        void addFiles (const QString &path);
        void clearFiles ();

        QString filename() const;
        QStringList selectedFilePaths();

        void setLocalData (const sfs::path& localData);

    private:

        void buildNewFileView();
        void buildOpenFileView();

    signals:

        void signalOpenFiles (const sfs::path &path);
        void signalCreateNewFile (const sfs::path &path);

        void signalUpdateAcceptButton (bool, int);

    private slots:

        void slotNewFile();
        void slotOpenFile();
        void slotUpdateAcceptButton (int);
        void slotUpdateAcceptButton (const QString &, bool);
        void slotRejected();
        void slotAddonDataChanged(const QModelIndex& topleft, const QModelIndex& bottomright);
    };
}
#endif // FILEDIALOG_HPP
