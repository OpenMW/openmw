#ifndef CSM_WOLRD_NESTEDTABLEWRAPPER_H
#define CSM_WOLRD_NESTEDTABLEWRAPPER_H

namespace CSMWorld
{
    struct NestedTableWrapperBase
    {
        virtual ~NestedTableWrapperBase();
        
        NestedTableWrapperBase();
    };

    template<typename NestedTable>
    struct NestedTableWrapper : public NestedTableWrapperBase
    {
        NestedTable mNestedTable;

        NestedTableWrapper(const NestedTable& nestedTable)
            : mNestedTable(nestedTable) {}

        virtual ~NestedTableWrapper() {}
    };
}
#endif
