#ifndef CSM_FILTER_NODE_H
#define CSM_FILTER_NODE_H

#include <string>
#include <map>
#include <vector>

#include <boost/shared_ptr.hpp>

#include <QMetaType>

namespace CSMWorld
{
    class IdTable;
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

            virtual bool test (const CSMWorld::IdTable& table, int row,
                const std::map<std::string, const Node *>& otherFilters,
                const std::map<int, int>& columns,
                const std::string& userValue) const = 0;
            ///< \return Can the specified table row pass through to filter?
            /// \param columns column ID to column index mapping

            virtual std::vector<std::string> getReferencedFilters() const = 0;
            ///< Return a list of filters that are used by this node (and must be passed as
            /// otherFilters when calling test).

            virtual std::vector<int> getReferencedColumns() const = 0;
            ///< Return a list of the IDs of the columns referenced by this node. The column mapping
            /// passed into test as columns must contain all columns listed here.

            virtual bool isSimple() const = 0;
            ///< \return Can this filter be displayed in simple mode.

            virtual bool hasUserValue() const = 0;

            virtual std::string toString (bool numericColumns) const = 0;
            ///< Return a string that represents this node.
            ///
            /// \param numericColumns Use numeric IDs instead of string to represent columns.
    };
}

Q_DECLARE_METATYPE (boost::shared_ptr<CSMFilter::Node>)

#endif
