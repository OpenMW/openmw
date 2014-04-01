
#include "pagedworldspacewidget.hpp"

#include <sstream>

CSVRender::PagedWorldspaceWidget::PagedWorldspaceWidget (QWidget *parent)
: WorldspaceWidget (parent), mMin (std::make_pair (0, 0)), mMax (std::make_pair (-1, -1))
{}

void CSVRender::PagedWorldspaceWidget::useViewHint (const std::string& hint)
{
    if (!hint.empty())
    {
        if (hint[0]=='c')
        {
            char ignore1, ignore2, ignore3;
            std::pair<int, int> cellIndex;

            std::istringstream stream (hint.c_str());
            if (stream >> ignore1 >> ignore2 >> ignore3 >> cellIndex.first >> cellIndex.second)
            {
                setCellIndex (cellIndex, cellIndex);

                /// \todo adjust camera position
            }
        }

        /// \todo implement 'r' type hints
    }
}

void CSVRender::PagedWorldspaceWidget::setCellIndex (const std::pair<int, int>& min,
    const std::pair<int, int>& max)
{
    mMin = min;
    mMax = max;

    emit cellIndexChanged (mMin, mMax);
}