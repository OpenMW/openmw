#ifndef CSV_WORLD_COLORPICKERDELEGATE_HPP
#define CSV_WORLD_COLORPICKERDELEGATE_HPP

#include "util.hpp"

class QRect;

namespace CSVWidget
{
    class ColorEditButton;
}

namespace CSVWorld
{
    class ColorPickerDelegate : public CommandDelegate
    {
            QRect getColoredRect(const QStyleOptionViewItem &option) const;

        public:
            ColorPickerDelegate(CSMWorld::CommandDispatcher *dispatcher, 
                                CSMDoc::Document& document, 
                                QObject *parent);

            virtual void paint(QPainter *painter, 
                               const QStyleOptionViewItem &option,
                               const QModelIndex &index) const;
    };

    class ColorPickerDelegateFactory : public CommandDelegateFactory
    {
        public:
            virtual CommandDelegate *makeDelegate(CSMWorld::CommandDispatcher *dispatcher, 
                                                  CSMDoc::Document &document, 
                                                  QObject *parent) const;
            ///< The ownership of the returned CommandDelegate is transferred to the caller.
    };
}

#endif
