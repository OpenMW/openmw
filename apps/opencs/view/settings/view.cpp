#include <QStandardItemModel>
#include <QStandardItem>
#include <QApplication>
#include <QItemSelectionModel>
#include <QStringListModel>

#include "view.hpp"
#include "../../model/settings/support.hpp"
#include "../../model/settings/setting.hpp"
#include "page.hpp"

CSVSettings::View::View(CSMSettings::Setting *setting,
                        Page *parent)

    : mDataModel(0), mParentPage (parent),
      mHasFixedValues (!setting->declaredValues().isEmpty()),
      mIsMultiValue (setting->isMultiValue()),
      mViewKey (setting->page() + '/' + setting->name()),
      mSerializable (setting->serializable()),
      Frame(true, setting->getLabel(), parent)
{
    if (!setting->getToolTip().isEmpty())
        setToolTip (setting->getToolTip());

    setObjectName (setting->name());
    buildView();
    buildModel (setting);
    // apply stylesheet to view's frame if exists
    if(setting->styleSheet() != "")
        Frame::setStyleSheet (setting->styleSheet());
}

void CSVSettings::View::buildModel (const CSMSettings::Setting *setting)
{
    QStringList values = setting->defaultValues();

    if (mHasFixedValues)
        buildFixedValueModel (setting->declaredValues());
    else
        buildUpdatableValueModel (values);

    mSelectionModel = new QItemSelectionModel (mDataModel, this);

    setSelectedValues (values, false);
}

void CSVSettings::View::buildFixedValueModel (const QStringList &values)
{
    //fixed value models are simple string list models, since they are read-only
    mDataModel = new QStringListModel (values, this);
}

void CSVSettings::View::buildUpdatableValueModel (const QStringList &values)
{
    //updateable models are standard item models because they support
    //replacing entire columns
    QList <QStandardItem *> itemList;

    foreach (const QString &value, values)
        itemList.append (new QStandardItem(value));

    QStandardItemModel *model = new QStandardItemModel (this);
    model->appendColumn (itemList);

    mDataModel = model;
}

void CSVSettings::View::buildView()
{
    setFlat (true);
    setHLayout();
}

int CSVSettings::View::currentIndex () const
{
    if (selectedValues().isEmpty())
        return -1;

    QString currentValue = selectedValues().at(0);

    for (int i = 0; i < mDataModel->rowCount(); i++)
        if (value(i) == currentValue)
            return i;

    return -1;
}

void CSVSettings::View::refresh() const
{
    select (mSelectionModel->selection());
    updateView();
}

int CSVSettings::View::rowCount() const
{
    return mDataModel->rowCount();
}

void CSVSettings::View::select (const QItemSelection &selection) const
{
    mSelectionModel->clear();
    mSelectionModel->select(selection, QItemSelectionModel::Select);
}

QStringList CSVSettings::View::selectedValues() const
{
    QStringList selValues;

    foreach (const QModelIndex &idx, mSelectionModel->selectedIndexes())
        selValues.append (value(idx.row()));

    return selValues;
}

void CSVSettings::View::setSelectedValue (const QString &value,
                                           bool doViewUpdate, bool signalUpdate)
{
    setSelectedValues (QStringList() << value, doViewUpdate, signalUpdate);
}

void CSVSettings::View::setSelectedValues (const QStringList &list,
                                           bool doViewUpdate, bool signalUpdate) const
{
    QItemSelection selection;

    if (stringListsMatch (list, selectedValues()))
        return;

    if (!mHasFixedValues)
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
                selection.append(QItemSelectionRange (idx, idx));
            }
        }
    }
    select (selection);

    //update the view if the selection was set from the model side, not by the
    //user
    if (doViewUpdate)
         updateView (signalUpdate);
}

void CSVSettings::View::showEvent ( QShowEvent * event )
{
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

void CSVSettings::View::updateView (bool signalUpdate) const
{
    if (signalUpdate)
        emit viewUpdated(viewKey(), selectedValues());
}

QString CSVSettings::View::value (int row) const
{
    if (row > -1 && row < mDataModel->rowCount())
        return mDataModel->data (mDataModel->index(row, 0)).toString();

    return QString();
}

int CSVSettings::View::widgetWidth(int characterCount) const
{
    QString widthToken = QString().fill ('m', characterCount);
    QFontMetrics fm (QApplication::font());

    return (fm.width (widthToken));
}
