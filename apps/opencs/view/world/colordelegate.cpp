#include "colordelegate.hpp"

#include <QPainter>
#include <QPushButton>

#include "../widget/coloreditor.hpp"

CSVWorld::ColorDelegate::ColorDelegate(CSMWorld::CommandDispatcher *dispatcher, 
                                       CSMDoc::Document& document, 
                                       QObject *parent)
    : CommandDelegate(dispatcher, document, parent)
{}

void CSVWorld::ColorDelegate::paint(QPainter *painter, 
                                    const QStyleOptionViewItem &option,
                                    const QModelIndex &index) const
{
    QRect coloredRect(qRound(option.rect.x() + option.rect.width() / 4.0),
                      qRound(option.rect.y() + option.rect.height() / 4.0),
                      qRound(option.rect.width() / 2.0),
                      qRound(option.rect.height() / 2.0));
    painter->save();
    painter->fillRect(coloredRect, index.data().value<QColor>());
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


