#ifndef CONTENTSELECTOR_HPP
#define CONTENTSELECTOR_HPP

#include <memory>

#include <QComboBox>
#include <QDialog>
#include <QMenu>
#include <QToolButton>

#include <components/contentselector/model/contentmodel.hpp>

class QSortFilterProxyModel;

namespace Ui
{
    class ContentSelector;
}

namespace ContentSelectorView
{
    class ContentSelector : public QObject
    {
        Q_OBJECT

        QMenu* mContextMenu;

    protected:
        ContentSelectorModel::ContentModel* mContentModel;
        QSortFilterProxyModel* mAddonProxyModel;

    public:
        explicit ContentSelector(QWidget* parent = nullptr, bool showOMWScripts = false);

        ~ContentSelector() override;

        QString currentFile() const;

        void addFiles(const QString& path, bool newfiles = false);
        void sortFiles();
        bool containsDataFiles(const QString& path);
        void clearFiles();
        void setNonUserContent(const QStringList& fileList);
        void setProfileContent(const QStringList& fileList);

        void clearCheckStates();
        void setEncoding(const QString& encoding);
        void setContentList(const QStringList& list);

        ContentSelectorModel::ContentFileList selectedFiles() const;

        void setGameFile(const QString& filename = QString(""));

        bool isGamefileSelected() const;

        QWidget* uiWidget() const;

        QComboBox* languageBox() const;

        QToolButton* refreshButton() const;

        QLineEdit* searchFilter() const;

    private:
        std::unique_ptr<Ui::ContentSelector> ui;

        void buildContentModel(bool showOMWScripts);
        void buildGameFileView();
        void buildAddonView();
        void buildContextMenu();
        void setGameFileSelected(int index, bool selected);
        void setCheckStateForMultiSelectedItems(bool checked);

    signals:
        void signalCurrentGamefileIndexChanged(int);

        void signalAddonDataChanged(const QModelIndex& topleft, const QModelIndex& bottomright);
        void signalSelectedFilesChanged(QStringList selectedFiles);

    private slots:

        void slotCurrentGameFileIndexChanged(int index);
        void slotAddonTableItemActivated(const QModelIndex& index);
        void slotShowContextMenu(const QPoint& pos);
        void slotCheckMultiSelectedItems();
        void slotUncheckMultiSelectedItems();
        void slotCopySelectedItemsPaths();
        void slotSearchFilterTextChanged(const QString& newText);
        void slotRowsMoved();
    };
}

#endif // CONTENTSELECTOR_HPP
