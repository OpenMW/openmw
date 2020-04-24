#include "enumdelegate.hpp"

#include <cassert>
#include <stdexcept>

#include <QComboBox>
#include <QApplication>
#include <QUndoStack>

#include "../../model/world/commands.hpp"

int CSVWorld::EnumDelegate::getValueIndex(const QModelIndex &index, int role) const
{
    if (index.isValid() && index.data(role).isValid())
    {
        int value = index.data(role).toInt();

        int size = static_cast<int>(mValues.size());
        for (int i = 0; i < size; ++i)
        {
            if (value == mValues.at(i).first)
            {
                return i;
            }
        }
    }
    return -1;
}

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
                // do nothing if the value has not changed
                if (model->data(index).toInt() != iter->first)
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
    CSMWorld::CommandDispatcher *dispatcher, CSMDoc::Document& document, QObject *parent)
: CommandDelegate (dispatcher, document, parent), mValues (values)
{

}

QWidget *CSVWorld::EnumDelegate::createEditor(QWidget *parent,
                                              const QStyleOptionViewItem& option,
                                              const QModelIndex& index) const
{
    return createEditor(parent, option, index, CSMWorld::ColumnBase::Display_None);
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
    if (QComboBox *comboBox = dynamic_cast<QComboBox *>(editor))
    {
        int role = Qt::EditRole;
        if (tryDisplay && !index.data(role).isValid())
        {
            role = Qt::DisplayRole;
            if (!index.data(role).isValid())
            {
                return;
            }
        }

        int valueIndex = getValueIndex(index, role);
        if (valueIndex != -1)
        {
            comboBox->setCurrentIndex(valueIndex);
        }
    }
}

void CSVWorld::EnumDelegate::paint (QPainter *painter, const QStyleOptionViewItem& option,
    const QModelIndex& index) const
{
    int valueIndex = getValueIndex(index);
    if (valueIndex != -1)
    {
#if QT_VERSION >= QT_VERSION_CHECK(5,7,0)
        QStyleOptionViewItem itemOption(option);
#else
        QStyleOptionViewItemV4 itemOption(option);
#endif
        itemOption.text = mValues.at(valueIndex).second;
        QApplication::style()->drawControl(QStyle::CE_ItemViewItem, &itemOption, painter);
    }
}

QSize CSVWorld::EnumDelegate::sizeHint(const QStyleOptionViewItem &option, const QModelIndex &index) const
{
    int valueIndex = getValueIndex(index);
    if (valueIndex != -1)
    {
        // Calculate the size hint as for a combobox.
        // So, the whole text is visible (isn't elided) when the editor is created
        QStyleOptionComboBox itemOption;
        itemOption.fontMetrics = option.fontMetrics;
        itemOption.palette = option.palette;
        itemOption.rect = option.rect;
        itemOption.state = option.state;

        const QString &valueText = mValues.at(valueIndex).second;

#if QT_VERSION >= QT_VERSION_CHECK(5,11,0)
        QSize valueSize = QSize(itemOption.fontMetrics.horizontalAdvance(valueText), itemOption.fontMetrics.height());
#else
        QSize valueSize = QSize(itemOption.fontMetrics.width(valueText), itemOption.fontMetrics.height());
#endif

        itemOption.currentText = valueText;
        return QApplication::style()->sizeFromContents(QStyle::CT_ComboBox, &itemOption, valueSize);
    }
    return option.rect.size();
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

CSVWorld::EnumDelegateFactory::EnumDelegateFactory (const std::vector<std::pair<int,std::string>>& names,
    bool allowNone)
{
    if (allowNone)
        add (-1, "");

    int size = static_cast<int> (names.size());

    for (int i=0; i<size; ++i)
        add (names[i].first, names[i].second.c_str());
}

CSVWorld::CommandDelegate *CSVWorld::EnumDelegateFactory::makeDelegate (
    CSMWorld::CommandDispatcher *dispatcher, CSMDoc::Document& document, QObject *parent) const
{
    return new EnumDelegate (mValues, dispatcher, document, parent);
}

void CSVWorld::EnumDelegateFactory::add (int value, const QString& name)
{
    auto pair = std::make_pair (value, name);

    for (auto it=mValues.begin(); it!=mValues.end(); ++it)
    {
        if (it->second > name)
        {
            mValues.insert(it, pair);
            return;
        }
    }

    mValues.push_back(std::make_pair (value, name));
}
