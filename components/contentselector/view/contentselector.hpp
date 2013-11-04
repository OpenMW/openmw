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

        QStringList mFilePaths;

    protected:

        ContentSelectorModel::ContentModel *mContentModel;
        QSortFilterProxyModel *mGameFileProxyModel;
        QSortFilterProxyModel *mAddonProxyModel;

    public:

        explicit ContentSelector(QWidget *parent = 0);

        QString currentFile() const;

        void addFiles(const QString &path);
        void setProfileContent (const QStringList &fileList);

        void clearCheckStates();
        void setCheckStates (const QStringList &list);

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

    signals:
        void signalCurrentGamefileIndexChanged (int);
        void signalAddonFileSelected (int);
        void signalAddonFileUnselected (int);

    private slots:

        void slotCurrentGameFileIndexChanged(int index);
        void slotAddonTableItemClicked(const QModelIndex &index);
    };
}

#endif // CONTENTSELECTOR_HPP
