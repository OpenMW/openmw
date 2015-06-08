#include "colorpickerdelegate.hpp"

#include <QPainter>
#include <QPushButton>

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

    return CommandDelegate::createEditor(parent, option, index, display);
}

void CSVWorld::ColorPickerDelegate::paint(QPainter *painter, 
                                          const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    QColor color = index.data().value<QColor>();
    QRect rect(option.rect.x() + option.rect.width() / 4,
               option.rect.y() + option.rect.height() / 4,
               option.rect.width() / 2,
               option.rect.height() / 2);
    
    painter->fillRect(rect, color);
}

CSVWorld::CommandDelegate *CSVWorld::ColorPickerDelegateFactory::makeDelegate(CSMWorld::CommandDispatcher *dispatcher, 
                                                                              CSMDoc::Document &document, 
                                                                              QObject *parent) const
{
    return new ColorPickerDelegate(dispatcher, document, parent);
}