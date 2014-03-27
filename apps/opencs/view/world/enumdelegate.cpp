
#include "enumdelegate.hpp"

#include <cassert>
#include <stdexcept>

#include <QComboBox>
#include <QApplication>
#include <QUndoStack>

#include "../../model/world/commands.hpp"

void CSVWorld::EnumDelegate::setModelDataImp (QWidget *editor, QAbstractItemModel *model,
    const QModelIndex& index) const
{
    if (QComboBox *comboBox = dynamic_cast<QComboBox *> (editor))
    {
        QString value = comboBox->currentText();

        for (std::vector<std::pair<int, QString> >::const_iterator iter (mValues.begin());
            iter!=mValues.end(); ++iter)
            if (iter->second==value)
            {
                addCommands (model, index, iter->first);
                break;
            }
    }
}

void CSVWorld::EnumDelegate::addCommands (QAbstractItemModel *model,
    const QModelIndex& index, int type) const
{
    getUndoStack().push (new CSMWorld::ModifyCommand (*model, index, type));
}


CSVWorld::EnumDelegate::EnumDelegate (const std::vector<std::pair<int, QString> >& values,
    QUndoStack& undoStack, QObject *parent)
: CommandDelegate (undoStack, parent), mValues (values)
{

}

QWidget *CSVWorld::EnumDelegate::createEditor(QWidget *parent,
                                              const QStyleOptionViewItem& option,
                                              const QModelIndex& index) const
{
    return createEditor(parent, option, index, CSMWorld::ColumnBase::Display_None);
    //overloading virtual functions is HARD
}

QWidget *CSVWorld::EnumDelegate::createEditor(QWidget *parent, const QStyleOptionViewItem& option,
    const QModelIndex& index, CSMWorld::ColumnBase::Display display) const
{
    if (!index.data(Qt::EditRole).isValid() && !index.data(Qt::DisplayRole).isValid())
        return 0;

    QComboBox *comboBox = new QComboBox (parent);

    for (std::vector<std::pair<int, QString> >::const_iterator iter (mValues.begin());
         iter!=mValues.end(); ++iter)
         comboBox->addItem (iter->second);

    return comboBox;
}

void CSVWorld::EnumDelegate::setEditorData (QWidget *editor, const QModelIndex& index, bool tryDisplay) const
{
    if (QComboBox *comboBox = dynamic_cast<QComboBox *> (editor))
    {
        QVariant data = index.data (Qt::EditRole);

        if (tryDisplay && !data.isValid())
        {
            data = index.data (Qt::DisplayRole);
            if (!data.isValid())
            {
                return;
            }
        }

        int value = data.toInt();

        std::size_t size = mValues.size();

        for (std::size_t i=0; i<size; ++i)
            if (mValues[i].first==value)
            {
                comboBox->setCurrentIndex (i);
                break;
            }
    }
}

void CSVWorld::EnumDelegate::paint (QPainter *painter, const QStyleOptionViewItem& option,
    const QModelIndex& index) const
{
    if (index.data().isValid())
    {
        QStyleOptionViewItemV4 option2 (option);

        int value = index.data().toInt();

        for (std::vector<std::pair<int, QString> >::const_iterator iter (mValues.begin());
            iter!=mValues.end(); ++iter)
            if (iter->first==value)
            {
                option2.text = iter->second;

                QApplication::style()->drawControl (QStyle::CE_ItemViewItem, &option2, painter);

                break;
            }
    }
}


CSVWorld::EnumDelegateFactory::EnumDelegateFactory() {}

CSVWorld::EnumDelegateFactory::EnumDelegateFactory (const char **names, bool allowNone)
{
    assert (names);

    if (allowNone)
        add (-1, "");

    for (int i=0; names[i]; ++i)
        add (i, names[i]);
}

CSVWorld::EnumDelegateFactory::EnumDelegateFactory (const std::vector<std::string>& names,
    bool allowNone)
{
    if (allowNone)
        add (-1, "");

    int size = static_cast<int> (names.size());

    for (int i=0; i<size; ++i)
        add (i, names[i].c_str());
}

CSVWorld::CommandDelegate *CSVWorld::EnumDelegateFactory::makeDelegate (QUndoStack& undoStack,
    QObject *parent) const
{
    return new EnumDelegate (mValues, undoStack, parent);
}

void CSVWorld::EnumDelegateFactory::add (int value, const QString& name)
{
    mValues.push_back (std::make_pair (value, name));
}
