#ifndef CSV_WORLD_VARTYPEDELEGATE_H
#define CSV_WORLD_VARTYPEDELEGATE_H

#include <components/esm/variant.hpp>

#include "enumdelegate.hpp"

namespace CSVWorld
{
    class VarTypeDelegate : public EnumDelegate
    {
        private:

            virtual void addCommands (QAbstractItemModel *model,
                const QModelIndex& index, int type) const;

        public:

            VarTypeDelegate (const std::vector<std::pair<int, QString> >& values,
                QUndoStack& undoStack, QObject *parent);
    };

    class VarTypeDelegateFactory : public CommandDelegateFactory
    {
            std::vector<std::pair<int, QString> > mValues;

        public:

            VarTypeDelegateFactory (ESM::VarType type0 = ESM::VT_Unknown,
                ESM::VarType type1 = ESM::VT_Unknown, ESM::VarType type2 = ESM::VT_Unknown,
                ESM::VarType type3 = ESM::VT_Unknown);

            virtual CommandDelegate *makeDelegate (QUndoStack& undoStack, QObject *parent) const;
            ///< The ownership of the returned CommandDelegate is transferred to the caller.

            void add (ESM::VarType type);
    };
}

#endif
