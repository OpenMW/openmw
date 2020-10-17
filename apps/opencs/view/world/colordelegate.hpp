#ifndef CSV_WORLD_COLORDELEGATE_HPP
#define CSV_WORLD_COLORDELEGATE_HPP

#include "util.hpp"

class QRect;

namespace CSVWidget
{
    class ColorEditButton;
}

namespace CSVWorld
{
    class ColorDelegate : public CommandDelegate
    {
        public:
            ColorDelegate(CSMWorld::CommandDispatcher *dispatcher, 
                          CSMDoc::Document& document, 
                          QObject *parent);

            void paint(QPainter *painter, 
                               const QStyleOptionViewItem &option,
                               const QModelIndex &index) const override;
    };

    class ColorDelegateFactory : public CommandDelegateFactory
    {
        public:
            CommandDelegate *makeDelegate(CSMWorld::CommandDispatcher *dispatcher, 
                                                  CSMDoc::Document &document, 
                                                  QObject *parent) const override;
            ///< The ownership of the returned CommandDelegate is transferred to the caller.
    };
}

#endif
