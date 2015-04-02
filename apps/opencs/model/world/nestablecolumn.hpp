#ifndef CSM_WOLRD_NESTABLECOLUMN_H
#define CSM_WOLRD_NESTABLECOLUMN_H

#include <vector>

#include "columnbase.hpp"

namespace CSMWorld
{
    class NestableColumn : public ColumnBase
    {
        std::vector<NestableColumn *> mNestedColumns;
        const NestableColumn* mParent;
        bool mHasChildren; // cached

    public:

        NestableColumn(int columnId,
                Display displayType, int flag, const NestableColumn* parent = 0);

        ~NestableColumn();

        void addColumn(CSMWorld::NestableColumn *column);

        const ColumnBase& nestedColumn(int subColumn) const;

        int nestedColumnCount() const;

        bool hasChildren() const;
    };
}

#endif
