#ifndef DATAFILESDIALOG_H
#define DATAFILESDIALOG_H

#include <QDialog>
#include <QModelIndex>
#include <QLineEdit>
#include <QPlainTextEdit>

#include "lineedit.h"

class DataFilesModel;
class QStringList;
class QTableView;
class QLineEdit;
class QPlainTextEdit;
class QItemSelectionModel;
class QSortFilterProxyModel;

class DataFilesDialog : public QDialog
{
    Q_OBJECT

public:
    DataFilesDialog();
//    ~DataFilesDialog() { };

private:
    DataFilesModel *dataFilesModel;

    QTableView *dataFilesView;
    QItemSelectionModel *selectionModel;
    QSortFilterProxyModel *sortModel;

    QLineEdit *lineAuthor;
    LineEdit *lineFilter;
    QPlainTextEdit *textDesc;
    QPlainTextEdit *textDepends;


public slots:

    void changeData(QModelIndex top, QModelIndex bottom); // edit
    void restoreDefaults();
    void readConfig();
    void writeConfig();
    void setupView();

    void setFilter();
    void setCheckstate(QModelIndex index);
//    void doubleClicked(QModelIndex index);

};

#endif
