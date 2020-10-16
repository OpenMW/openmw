#ifndef CSM_FILTER_VALUENODE_H
#define CSM_FILTER_VALUENODE_H

#include "leafnode.hpp"

namespace CSMFilter
{
    class ValueNode : public LeafNode
    {
        public:

            enum Type
            {
                Type_Closed, Type_Open, Type_Infinite
            };

        private:

            int mColumnId;
            std::string mText;
            double mLower;
            double mUpper;
            Type mLowerType;
            Type mUpperType;

        public:

            ValueNode (int columnId, Type lowerType, Type upperType, double lower, double upper);

            bool test (const CSMWorld::IdTableBase& table, int row,
                const std::map<int, int>& columns) const override;
            ///< \return Can the specified table row pass through to filter?
            /// \param columns column ID to column index mapping

            std::vector<int> getReferencedColumns() const override;
            ///< Return a list of the IDs of the columns referenced by this node. The column mapping
            /// passed into test as columns must contain all columns listed here.

            std::string toString (bool numericColumns) const override;
            ///< Return a string that represents this node.
            ///
            /// \param numericColumns Use numeric IDs instead of string to represent columns.
    };
}

#endif
