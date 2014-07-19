#ifndef CSM_WOLRD_NESTEDTABLEWRAPPER_H
#define CSM_WOLRD_NESTEDTABLEWRAPPER_H

#include <components/esm/loadcont.hpp>

#include <vector>
namespace CSMWorld
{
    struct NestedTableWrapperBase
    {
        virtual ~NestedTableWrapperBase();
        
        NestedTableWrapperBase();
    };

    template<typename NestedTable>
    class NestedTableWrapper : public NestedTableWrapperBase
    {
        NestedTable mNestedTable;

    public:
        
        NestedTableWrapper(const NestedTable& nestedTable) 
            : mNestedTable(nestedTable) {}

        NestedTable getNestedTable() const
        {
            return mNestedTable;
        }

        virtual ~NestedTableWrapper() {}
    };
}
#endif
