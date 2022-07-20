#ifndef CSV_WORLD_VARTYPEDELEGATE_H
#define CSV_WORLD_VARTYPEDELEGATE_H

#include <components/esm3/variant.hpp>

#include "enumdelegate.hpp"

namespace CSVWorld
{
    class VarTypeDelegate : public EnumDelegate
    {
        private:

            void addCommands (QAbstractItemModel *model,
                const QModelIndex& index, int type) const override;

        public:

            VarTypeDelegate (const std::vector<std::pair<int, QString> >& values,
                CSMWorld::CommandDispatcher *dispatcher, CSMDoc::Document& document, QObject *parent);
    };

    class VarTypeDelegateFactory : public CommandDelegateFactory
    {
            std::vector<std::pair<int, QString> > mValues;

        public:

            VarTypeDelegateFactory (ESM::VarType type0 = ESM::VT_Unknown,
                ESM::VarType type1 = ESM::VT_Unknown, ESM::VarType type2 = ESM::VT_Unknown,
                ESM::VarType type3 = ESM::VT_Unknown);

            CommandDelegate *makeDelegate (CSMWorld::CommandDispatcher *dispatcher,
                CSMDoc::Document& document, QObject *parent) const override;
            ///< The ownership of the returned CommandDelegate is transferred to the caller.

            void add (ESM::VarType type);
    };
}

#endif
