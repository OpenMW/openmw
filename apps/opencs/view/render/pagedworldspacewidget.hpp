#ifndef OPENCS_VIEW_PAGEDWORLDSPACEWIDGET_H
#define OPENCS_VIEW_PAGEDWORLDSPACEWIDGET_H

#include "worldspacewidget.hpp"

namespace CSVRender
{
    class PagedWorldspaceWidget : public WorldspaceWidget
    {
            Q_OBJECT

            std::pair<int, int> mMin;
            std::pair<int, int> mMax;

        public:

            PagedWorldspaceWidget (QWidget *parent);
            ///< \note Sets the cell area selection to an invalid value to indicate that currently
            /// no cells are displayed. The cells to be displayed will be specified later through
            /// hint system.

            virtual void useViewHint (const std::string& hint);

            void setCellIndex (const std::pair<int, int>& min, const std::pair<int, int>& max);

        signals:

            void cellIndexChanged (const std::pair<int, int>& min, const std::pair<int, int>& max);
    };
}

#endif
