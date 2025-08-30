#ifndef CSM_WOLRD_NESTEDTABLEWRAPPER_H
#define CSM_WOLRD_NESTEDTABLEWRAPPER_H

namespace CSMWorld
{
    struct NestedTableWrapperBase
    {
        virtual ~NestedTableWrapperBase() = default;

        virtual int size() const { return -5; }

        NestedTableWrapperBase() = default;
    };

    template <typename NestedTable>
    struct NestedTableWrapper : public NestedTableWrapperBase
    {
        NestedTable mNestedTable;

        NestedTableWrapper(const NestedTable& nestedTable)
            : mNestedTable(nestedTable)
        {
        }

        ~NestedTableWrapper() override = default;

        int size() const override
        {
            return mNestedTable.size(); // i hope that this will be enough
        }
    };
}
#endif
