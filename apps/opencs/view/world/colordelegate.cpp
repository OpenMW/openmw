#include "colordelegate.hpp"

#include <QPainter>
#include <QPushButton>

CSVWorld::ColorDelegate::ColorDelegate(CSMWorld::CommandDispatcher *dispatcher,
                                       CSMDoc::Document& document,
                                       QObject *parent)
    : CommandDelegate(dispatcher, document, parent)
{}

void CSVWorld::ColorDelegate::paint(QPainter *painter,
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    int colorInt = index.data().toInt();
    QColor color(colorInt & 0xff, (colorInt >> 8) & 0xff, (colorInt >> 16) & 0xff);

    QRect coloredRect(option.rect.x() + qRound(option.rect.width() / 4.0),
                      option.rect.y() + qRound(option.rect.height() / 4.0),
                      option.rect.width() / 2,
                      option.rect.height() / 2);
    painter->save();
    painter->fillRect(coloredRect, color);
    painter->setPen(Qt::black);
    painter->drawRect(coloredRect);
    painter->restore();
}

CSVWorld::CommandDelegate *CSVWorld::ColorDelegateFactory::makeDelegate(CSMWorld::CommandDispatcher *dispatcher,
                                                                        CSMDoc::Document &document,
                                                                        QObject *parent) const
{
    return new ColorDelegate(dispatcher, document, parent);
}


