#ifndef CSV_WORLD_COLORPICKERDELEGATE_HPP
#define CSV_WORLD_COLORPICKERDELEGATE_HPP

#include "util.hpp"

namespace CSVWorld
{
    class ColorPickerDelegate : public CommandDelegate
    {
        public:
            ColorPickerDelegate(CSMWorld::CommandDispatcher *dispatcher, 
                                CSMDoc::Document& document, 
                                QObject *parent);

            virtual QWidget *createEditor(QWidget *parent,
                                          const QStyleOptionViewItem &option,
                                          const QModelIndex &index) const;

            virtual QWidget *createEditor(QWidget *parent,
                                          const QStyleOptionViewItem &option,
                                          const QModelIndex &index,
                                          CSMWorld::ColumnBase::Display display) const;

            virtual void paint(QPainter *painter, 
                               const QStyleOptionViewItem &option,
                               const QModelIndex &index) const;/*

            virtual void setEditorData(QWidget *editor, const QModelIndex &index) const;

            virtual void setModelData(QWidget *editor, 
                                      QAbstractItemModel &model, 
                                      const QModelIndex &index) const;*/
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
