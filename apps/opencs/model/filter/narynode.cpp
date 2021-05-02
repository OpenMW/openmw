#include "narynode.hpp"

#include <sstream>

CSMFilter::NAryNode::NAryNode (const std::vector<std::shared_ptr<Node> >& nodes,
    const std::string& name)
: mNodes (nodes), mName (name)
{}

int CSMFilter::NAryNode::getSize() const
{
    return static_cast<int>(mNodes.size());
}

const CSMFilter::Node& CSMFilter::NAryNode::operator[] (int index) const
{
    return *mNodes.at (index);
}

std::vector<int> CSMFilter::NAryNode::getReferencedColumns() const
{
    std::vector<int> columns;

    for (std::vector<std::shared_ptr<Node> >::const_iterator iter (mNodes.begin());
         iter!=mNodes.end(); ++iter)
    {
        std::vector<int> columns2 = (*iter)->getReferencedColumns();

        columns.insert (columns.end(), columns2.begin(), columns2.end());
    }

    return columns;
}

std::string CSMFilter::NAryNode::toString (bool numericColumns) const
{
    std::ostringstream stream;

    stream << mName << " (";

    bool first = true;
    int size = getSize();

    for (int i=0; i<size; ++i)
    {
        if (first)
            first = false;
        else
            stream << ", ";

        stream << (*this)[i].toString (numericColumns);
    }

    stream << ")";

    return stream.str();
}


