#ifndef CONTENTSELECTOR_HPP
#define CONTENTSELECTOR_HPP

#include <QDialog>

#include "ui_contentselector.h"
#include "../model/contentmodel.hpp"

class QSortFilterProxyModel;

namespace ContentSelectorView
{
    class ContentSelector : public QObject
    {
        Q_OBJECT

        QMenu *mContextMenu;
        QStringList mFilePaths;

    protected:

        ContentSelectorModel::ContentModel *mContentModel;
        QSortFilterProxyModel *mAddonProxyModel;

    public:

        explicit ContentSelector(QWidget *parent = 0);

        QString currentFile() const;

        void addFiles(const QString &path);
        void setProfileContent (const QStringList &fileList);

        void clearCheckStates();
        void setContentList(const QStringList &list);

        ContentSelectorModel::ContentFileList selectedFiles() const;

        void setGameFile (const QString &filename = QString(""));

        bool isGamefileSelected() const
            { return ui.gameFileView->currentIndex() != -1; }

        QWidget *uiWidget() const
            { return ui.contentGroupBox; }


   private:

        Ui::ContentSelector ui;

        void buildContentModel();
        void buildGameFileView();
        void buildAddonView();
        void buildContextMenu();
        void setGameFileSelected(int index, bool selected);
        void setCheckStateForMultiSelectedItems(bool checked);

    signals:
        void signalCurrentGamefileIndexChanged (int);

        void signalAddonDataChanged (const QModelIndex& topleft, const QModelIndex& bottomright);

    private slots:

        void slotCurrentGameFileIndexChanged(int index);
        void slotAddonTableItemActivated(const QModelIndex& index);
        void slotShowContextMenu(const QPoint& pos);
        void slotCheckMultiSelectedItems();
        void slotUncheckMultiSelectedItems();
    };
}

#endif // CONTENTSELECTOR_HPP
