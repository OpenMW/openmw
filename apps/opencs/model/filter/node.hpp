#ifndef CSM_FILTER_NODE_H
#define CSM_FILTER_NODE_H

#include <string>
#include <map>
#include <memory>
#include <vector>

#include <QMetaType>

namespace CSMWorld
{
    class IdTableBase;
}

namespace CSMFilter
{
    /// \brief Root class for the filter node hierarchy
    ///
    /// \note When the function documentation for this class mentions "this node", this should be
    /// interpreted as "the node and all its children".
    class Node
    {
            // not implemented
            Node (const Node&);
            Node& operator= (const Node&);

        public:

            Node();

            virtual ~Node();

            virtual bool test (const CSMWorld::IdTableBase& table, int row,
                const std::map<int, int>& columns) const = 0;
            ///< \return Can the specified table row pass through to filter?
            /// \param columns column ID to column index mapping

            virtual std::vector<int> getReferencedColumns() const = 0;
            ///< Return a list of the IDs of the columns referenced by this node. The column mapping
            /// passed into test as columns must contain all columns listed here.

            virtual std::string toString (bool numericColumns) const = 0;
            ///< Return a string that represents this node.
            ///
            /// \param numericColumns Use numeric IDs instead of string to represent columns.
    };
}

Q_DECLARE_METATYPE (std::shared_ptr<CSMFilter::Node>)

#endif
