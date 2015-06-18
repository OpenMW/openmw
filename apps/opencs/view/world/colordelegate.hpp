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

            virtual void paint(QPainter *painter, 
                               const QStyleOptionViewItem &option,
                               const QModelIndex &index) const;
    };

    class ColorDelegateFactory : public CommandDelegateFactory
    {
        public:
            virtual CommandDelegate *makeDelegate(CSMWorld::CommandDispatcher *dispatcher, 
                                                  CSMDoc::Document &document, 
                                                  QObject *parent) const;
            ///< The ownership of the returned CommandDelegate is transferred to the caller.
    };
}

#endif
