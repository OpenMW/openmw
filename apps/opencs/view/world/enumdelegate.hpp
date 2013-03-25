#ifndef CSV_WORLD_ENUMDELEGATE_H
#define CSV_WORLD_ENUMDELEGATE_H

#include <vector>

#include <QString>

#include <components/esm/defs.hpp>

#include "util.hpp"

namespace CSVWorld
{
    /// \brief Integer value that represents an enum and is interacted with via a combobox
    class EnumDelegate : public CommandDelegate
    {
            std::vector<std::pair<int, QString> > mValues;

        private:

            virtual void setModelDataImp (QWidget *editor, QAbstractItemModel *model,
                const QModelIndex& index) const;

            virtual void addCommands (QAbstractItemModel *model,
                const QModelIndex& index, int type) const;

        public:

            EnumDelegate (const std::vector<std::pair<int, QString> >& values,
                QUndoStack& undoStack, QObject *parent);

            virtual QWidget *createEditor(QWidget *parent, const QStyleOptionViewItem& option,
                const QModelIndex& index) const;

            virtual void setEditorData (QWidget *editor, const QModelIndex& index) const;

            virtual void paint (QPainter *painter, const QStyleOptionViewItem& option,
                const QModelIndex& index) const;

    };

    class EnumDelegateFactory : public CommandDelegateFactory
    {
            std::vector<std::pair<int, QString> > mValues;

        public:

            EnumDelegateFactory();

            EnumDelegateFactory (const char **names);
            ///< \param names Array of char pointer with a 0-pointer as end mark

            virtual CommandDelegate *makeDelegate (QUndoStack& undoStack, QObject *parent) const;
            ///< The ownership of the returned CommandDelegate is transferred to the caller.

            void add (int value, const QString& name);
    };


}

#endif