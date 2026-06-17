#ifndef CSM_WOLRD_NESTEDTABLEWRAPPER_H
#define CSM_WOLRD_NESTEDTABLEWRAPPER_H

namespace CSMWorld
{
    struct NestedTableWrapperBase
    {
        virtual ~NestedTableWrapperBase() = default;

        virtual size_t size() const = 0;
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

        size_t size() const override
        {
            return mNestedTable.size(); // i hope that this will be enough
        }
    };
}
#endif
