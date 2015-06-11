#include "colorpickerdelegate.hpp"

#include <QPainter>
#include <QPushButton>

#include "../widget/coloreditor.hpp"

CSVWorld::ColorPickerDelegate::ColorPickerDelegate(CSMWorld::CommandDispatcher *dispatcher, 
                                                   CSMDoc::Document& document, 
                                                   QObject *parent)
    : CommandDelegate(dispatcher, document, parent)
{}

QWidget *CSVWorld::ColorPickerDelegate::createEditor(QWidget *parent,
                                                     const QStyleOptionViewItem &option,
                                                     const QModelIndex &index) const
{
    return createEditor(parent, option, index, getDisplayTypeFromIndex(index));
}

QWidget *CSVWorld::ColorPickerDelegate::createEditor(QWidget *parent,
                                                     const QStyleOptionViewItem &option,
                                                     const QModelIndex &index,
                                                     CSMWorld::ColumnBase::Display display) const
{
    if (display != CSMWorld::ColumnBase::Display_Colour)
    {
        throw std::logic_error("Wrong column for ColorPickerDelegate");
    }

    return new CSVWidget::ColorEditor(index.data().value<QColor>(), 
                                      getColoredRect(option).size(),
                                      parent);
}

void CSVWorld::ColorPickerDelegate::paint(QPainter *painter, 
                                          const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    painter->fillRect(getColoredRect(option), index.data().value<QColor>());
}

QRect CSVWorld::ColorPickerDelegate::getColoredRect(const QStyleOptionViewItem &option) const
{
    return QRect(option.rect.x() + option.rect.width() / 4,
                 option.rect.y() + option.rect.height() / 4,
                 option.rect.width() / 2,
                 option.rect.height() / 2);
}

CSVWorld::CommandDelegate *CSVWorld::ColorPickerDelegateFactory::makeDelegate(CSMWorld::CommandDispatcher *dispatcher, 
                                                                              CSMDoc::Document &document, 
                                                                              QObject *parent) const
{
    return new ColorPickerDelegate(dispatcher, document, parent);
}


