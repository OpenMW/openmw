#include "colorpickerdelegate.hpp"

#include <QPainter>
#include <QPushButton>

#include "../widget/coloreditor.hpp"

CSVWorld::ColorPickerDelegate::ColorPickerDelegate(CSMWorld::CommandDispatcher *dispatcher, 
                                                   CSMDoc::Document& document, 
                                                   QObject *parent)
    : CommandDelegate(dispatcher, document, parent)
{}

void CSVWorld::ColorPickerDelegate::paint(QPainter *painter, 
                                          const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const
{
    QRect coloredRect = getColoredRect(option);
    painter->fillRect(coloredRect, index.data().value<QColor>());
}

QRect CSVWorld::ColorPickerDelegate::getColoredRect(const QStyleOptionViewItem &option) const
{
    return QRect(qRound(option.rect.x() + option.rect.width() / 4.0),
                 qRound(option.rect.y() + option.rect.height() / 4.0),
                 qRound(option.rect.width() / 2.0),
                 qRound(option.rect.height() / 2.0));
}

CSVWorld::CommandDelegate *CSVWorld::ColorPickerDelegateFactory::makeDelegate(CSMWorld::CommandDispatcher *dispatcher, 
                                                                              CSMDoc::Document &document, 
                                                                              QObject *parent) const
{
    return new ColorPickerDelegate(dispatcher, document, parent);
}


