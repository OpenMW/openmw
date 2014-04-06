#include <QHBoxLayout>
#include <QVBoxLayout>
#include <QGroupBox>
#include <QStringListModel>
#include <QSortFilterProxyModel>
#include <QStandardItem>

#include "view.hpp"
#include "support.hpp"
#include "../../model/settings/setting.hpp"
#include "settingbox.hpp"
#include "page.hpp"

#include <QDebug>
CSVSettings::View::View(CSMSettings::Setting *setting,
                        Page *parent)

    : mSetting (setting), mDataModel(0), mParentPage (parent),
      mHasDeclaredValues (!setting->declaredValues().isEmpty()),
      QWidget(parent)
{
    setObjectName (setting->name());
    buildView();
    buildModel();
}

void CSVSettings::View::buildModel()
{
    QStringList definitions = mSetting->definedValues();

    if (definitions.isEmpty())
        definitions.append (mSetting->defaultValues());

    qDebug() << objectName() << "::View::buildModel() definitions: " << definitions;

    if (mHasDeclaredValues)
        mDataModel = new QStringListModel (mSetting->declaredValues(), this);
    else
    {
        QList <QStandardItem *> itemList;

        foreach (const QString &value, definitions)
            itemList.append (new QStandardItem(value));

//        QSortFilterProxyModel *filter = new QSortFilterProxyModel (this);
        QStandardItemModel *model = new QStandardItemModel (this);
        model->appendColumn (itemList);

  //      filter->setSourceModel (model);
     /*   filter->setFilterRegExp ("*");
        filter->setFilterKeyColumn (0);
        filter->setFilterRole (Qt::DisplayRole);*/

        mDataModel = model;
    }

    mSelectionModel = new QItemSelectionModel (mDataModel, this);

    setSelectedValues (definitions, false);
}

void CSVSettings::View::buildView()
{
    mViewFrame = new SettingBox(true, mSetting->name(), this);
    mViewFrame->setFlat (true);
    mViewFrame->setHLayout();
}

void CSVSettings::View::refresh() const
{
    QItemSelection selection = mSelectionModel->selection();
    qDebug() << objectName() << "::View::refresh() selection count " << selection.size();
    select (selection);
}

void CSVSettings::View::select (const QItemSelection &selection) const
{
    mSelectionModel->clear();
    mSelectionModel->select(selection, QItemSelectionModel::Select);
    updateView();
}

QStringList CSVSettings::View::selectedValues() const
{
    QStringList selValues;

    foreach (const QModelIndex &idx, mSelectionModel->selectedIndexes())
        selValues.append (value(idx.row()));

    return selValues;
}

void CSVSettings::View::setSelectedValues (const QStringList &list,
                                           bool doViewUpdate)
{
    QItemSelection selection;

    if (stringListsMatch (list, selectedValues()))
        return;

    if (!mHasDeclaredValues)
    {
        QStandardItemModel *model  =
                                static_cast <QStandardItemModel *>(mDataModel);

        model->clear();
        model->appendColumn (toStandardItemList (list));

        for (int i = 0; i < model->rowCount(); i++)
        {
            QModelIndex idx = model->index(i, 0);
            selection.append (QItemSelectionRange (idx, idx));
        }
    }
    else
    {
        for (int i = 0; i < mDataModel->rowCount(); i++)
        {
            if (list.contains(value(i)))
            {
                QModelIndex idx = mDataModel->index(i, 0);
                selection.append(QItemSelectionRange ((idx, idx)));
            }
        }
    }
    select (selection);

    if (doViewUpdate)
        updateView();
}

void CSVSettings::View::showEvent ( QShowEvent * event )
{
    if (mSetting->proxyLists().isEmpty());
        refresh();
}

bool CSVSettings::View::stringListsMatch (
                                                const QStringList &list1,
                                                const QStringList &list2) const
{
    //returns a "sloppy" match, verifying that each list contains all the same
    //items, though not necessarily in the same order.

    if (list1.size() != list2.size())
        return false;

    QStringList tempList(list2);

    //iterate each value in the list, removing one occurrence of the value in
    //the other list.  If no corresponding value is found, test fails
    foreach (const QString &value, list1)
    {
        if (!tempList.contains(value))
            return false;

        tempList.removeOne(value);
    }
    return true;
}

QList <QStandardItem *> CSVSettings::View::toStandardItemList
                                                (const QStringList &list) const
{
    QList <QStandardItem *> itemList;

    foreach (const QString &value, list)
        itemList.append (new QStandardItem (value));

    return itemList;
}

void CSVSettings::View::updateView() const
{
    emit viewUpdated();
}

QString CSVSettings::View::value (int row) const
{
    QString value = "";

    if (row > -1 && row < mDataModel->rowCount())
        value = mDataModel->data (mDataModel->index(row, 0)).toString();

    return value;
}
