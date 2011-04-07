#ifndef DATAFILESDIALOG_H
#define DATAFILESDIALOG_H

#include "lineedit.h"

//class DataFilesModel;


/*class QStringList;
class QTreeView;
class QLineEdit;
class QPlainTextEdit;
class QItemSelectionModel;
class QSortFilterProxyModel;
*/
#include <QDialog>
#include <QModelIndex>

class QTreeView;
class QTableView;
class QStandardItemModel;
class QItemSelectionModel;
class QItemSelection;

class DataFilesDialog : public QDialog
{
    Q_OBJECT

public:
    DataFilesDialog();
//    ~DataFilesDialog() { };

private:
    //QStandardItemModel *dataFilesModel;

    //QTreeView *dataFilesView;
    //QItemSelectionModel *selectionModel;
    //QSortFilterProxyModel *sortModel;

    //QLineEdit *lineAuthor;
    //LineEdit *lineFilter;
    //QPlainTextEdit *textDesc;
    //QPlainTextEdit *textDepends;
    QModelIndexList *masterindexes;

    QStandardItemModel *datafilesmodel;
    QStandardItemModel *mastersmodel;
    QStandardItemModel *pluginsmodel;
    
    QItemSelectionModel *masterselectmodel;
    QItemSelectionModel *pluginselectmodel;
    
    QTreeView *tree;

    QTableView *mastertable;
    QTableView *plugintable;

    void appendPlugins(const QModelIndex &masterindex);
    void removePlugins(const QModelIndex &masterindex);

public slots:

    //void changeData(QModelIndex top, QModelIndex bottom); // edit
    void restoreDefaults();
    void readConfig();
    void writeConfig();
    void showContextMenu(const QPoint &point);
    
    void actionCheckstate();
    void test();
    //void setupView();

    void masterSelectionChanged(const QItemSelection &selected, const QItemSelection &deselected);
    //void setFilter();
    void setCheckstate(QModelIndex index);
//    void doubleClicked(QModelIndex index);

};

#endif
