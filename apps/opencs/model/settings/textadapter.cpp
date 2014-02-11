#include "textadapter.hpp"
#include "setting.hpp"
#include "definitionmodel.hpp"

#include <QSortFilterProxyModel>
#include <QDebug>
#include <QStandardItemModel>

CSMSettings::TextAdapter::TextAdapter(QStandardItemModel &model,
                                      const CSMSettings::Setting *setting,
                                      QObject *parent)
    : mDelimiter (setting->delimiter()),
      Adapter (model, setting->page(), setting->name(), setting->isMultiValue()
               , parent)
{
    if (setting->isMultiLine())
        mDelimiter = '\n';
}

QVariant CSMSettings::TextAdapter::data (const QModelIndex &index,
                                         int role) const
{
    qDebug() << "TextAdapter::data()";

    if (!validIndex(index))
        return QVariant();

    QString concatenation;

    switch (role)
    {
    case Qt::EditRole:
    case Qt::DisplayRole:

        if (!isMultiValue())
        {
            qDebug() << "returning single-value...";
            concatenation = filter()->data (
                                filter()->index (
                                    filter()->rowCount()-1, 2
                                )
                            ).toString();
            qDebug() << "\t" << concatenation;
            return concatenation;
        }
        qDebug() << "\t multi-valued, delimiter: " << mDelimiter;
        //iterate each row in the filter and concatenate the values into
        //a string, delimited by the user-defined delimiter.
        for (int i = 0; i < filter()->rowCount(); i++)
        {
            QModelIndex idx = filter()->index (i, 2);

            if (!concatenation.isEmpty())
                concatenation += mDelimiter;

            concatenation += filter()->data(idx).toString();
        }
        qDebug() << "\treturning concat: " << concatenation;
        return concatenation;

    break;

    default:
    break;
    }

    return QVariant();
}

bool CSMSettings::TextAdapter::setData (const QModelIndex &index,
                                        const QVariant &value,
                                        int role)
{
    qDebug() << "TextAdapter::setData()";

    QStringList valueList =
                value.toString().split (mDelimiter, QString::SkipEmptyParts);\

    qDebug() << "\tvalueList: " << valueList;

    if (valueList.size() == 0)
        return false;

    //if the user tried to enter multiple values into a single-valued setting,
    //erase all but the last value

    if (!isMultiValue())
    {
        if (valueList.size() > 1)
        {
            QStringList tempList = valueList;
            valueList.clear();
            valueList.append (tempList.at(tempList.size() - 1));
        }
    }

    bool isDifferent = false;

    //check to make sure there are differences between the value list and the
    //model definitions
    for (int i = 0; i < filter()->rowCount(); i++)
    {
        QModelIndex idx = filter()->index(i, 2);
        isDifferent = !valueList.contains(filter()->data (idx).toString());

        if (isDifferent)
            break;
    }

    if (!isDifferent)
    {
        bool valueFound = false;
        qDebug() << "\tpassed check #1";
        //check the value list against the model.
        foreach (const QString &value, valueList)
        {
            valueFound = false;
            for (int i = 0; i < filter()->rowCount(); i++)
            {
                valueFound = (value == filter()->data (
                                  filter()->index (i, 2)).toString());

                if (valueFound)
                    break;
            }
            if (!valueFound)
                break;
        }
        if (valueFound)
            return false;
    }
    qDebug() << "\tpassed check #2";
    //clear and repopulate the model from the value list
    filter()->removeRows (0, filter()->rowCount());
    qDebug() << "\tAdding values: " << valueList;
    foreach (const QString &value, valueList)
    {
        qDebug() << "\tinserting value: " << value;
       // model().defineSetting (settingName(), pageName(), value);
    }
    return true;
}

void CSMSettings::TextAdapter::slotLayoutChanged()
{
    QModelIndex idx = index(0, 2, QModelIndex());
    emit dataChanged (idx, idx);
}

int CSMSettings::TextAdapter::rowCount(const QModelIndex &parent) const
{
    return 1;
}

int CSMSettings::TextAdapter::columnCount (const QModelIndex &parent) const
{
    return 3;
}

