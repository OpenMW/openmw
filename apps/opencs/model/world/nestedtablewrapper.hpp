#ifndef CSM_WOLRD_NESTEDTABLEWRAPPER_H
#define CSM_WOLRD_NESTEDTABLEWRAPPER_H

namespace CSMWorld
{
    struct NestedTableWrapperBase
    {
        virtual ~NestedTableWrapperBase();
        
        virtual int size() const;
        
        NestedTableWrapperBase();
    };
    
    template<typename NestedTable>
    struct NestedTableWrapper : public NestedTableWrapperBase
    {
        NestedTable mNestedTable;

        NestedTableWrapper(const NestedTable& nestedTable)
            : mNestedTable(nestedTable) {}

        ~NestedTableWrapper() override {}

        int size() const override
        {
            return mNestedTable.size(); //i hope that this will be enough
        }
    };
}
#endif
