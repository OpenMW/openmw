#ifndef CSV_WORLD_IDCOMPLETIONDELEGATE_HPP
#define CSV_WORLD_IDCOMPLETIONDELEGATE_HPP

#include "util.hpp"

namespace CSVWorld
{
    /// \brief Enables the Id completion for a column
    class IdCompletionDelegate : public CommandDelegate
    {
        public:
            IdCompletionDelegate(CSMWorld::CommandDispatcher *dispatcher, 
                                 CSMDoc::Document& document, 
                                 QObject *parent);

            QWidget *createEditor (QWidget *parent,
                                           const QStyleOptionViewItem &option,
                                           const QModelIndex &index) const override;

            QWidget *createEditor (QWidget *parent,
                                           const QStyleOptionViewItem &option,
                                           const QModelIndex &index,
                                           CSMWorld::ColumnBase::Display display) const override;
    };

    class IdCompletionDelegateFactory : public CommandDelegateFactory
    {
        public:
            CommandDelegate *makeDelegate(CSMWorld::CommandDispatcher *dispatcher, 
                                                  CSMDoc::Document& document, 
                                                  QObject *parent) const override;
            ///< The ownership of the returned CommandDelegate is transferred to the caller.
    };
}

#endif
