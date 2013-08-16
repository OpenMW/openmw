
#include "narynode.hpp"

CSMFilter::NAryNode::NAryNode (const std::vector<boost::shared_ptr<Node> >& nodes)
: mNodes (nodes)
{}

int CSMFilter::NAryNode::getSize() const
{
    return mNodes.size();
}

const CSMFilter::Node& CSMFilter::NAryNode::operator[] (int index) const
{
    return *mNodes.at (index);
}

std::vector<std::string> CSMFilter::NAryNode::getReferencedFilters() const
{
    std::vector<std::string> filters;

    for (std::vector<boost::shared_ptr<Node> >::const_iterator iter (mNodes.begin());
         iter!=mNodes.end(); ++iter)
    {
        std::vector<std::string> filters2 = (*iter)->getReferencedFilters();

        filters.insert (filters.end(), filters2.begin(), filters2.end());
    }

    return filters;
}

std::vector<int> CSMFilter::NAryNode::getReferencedColumns() const
{
    std::vector<int> columns;

    for (std::vector<boost::shared_ptr<Node> >::const_iterator iter (mNodes.begin());
         iter!=mNodes.end(); ++iter)
    {
        std::vector<int> columns2 = (*iter)->getReferencedColumns();

        columns.insert (columns.end(), columns2.begin(), columns2.end());
    }

    return columns;
}

bool CSMFilter::NAryNode::isSimple() const
{
    return false;
}

bool CSMFilter::NAryNode::hasUserValue() const
{
    for (std::vector<boost::shared_ptr<Node> >::const_iterator iter (mNodes.begin());
         iter!=mNodes.end(); ++iter)
         if ((*iter)->hasUserValue())
             return true;

    return false;
}
