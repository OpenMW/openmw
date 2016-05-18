#include "pathgridmode.hpp"

#include <QMenu>
#include <QPoint>

#include <components/sceneutil/pathgridutil.hpp>

#include "../../model/world/commands.hpp"
#include "../../model/world/commandmacro.hpp"
#include "../../model/world/idtable.hpp"
#include "../../model/world/idtree.hpp"

#include "../widget/scenetoolbar.hpp"

#include "cell.hpp"
#include "mask.hpp"
#include "pathgrid.hpp"
#include "pathgridselectionmode.hpp"
#include "worldspacewidget.hpp"

namespace CSVRender
{
    PathgridMode::PathgridMode(WorldspaceWidget* worldspaceWidget, QWidget* parent)
        : EditMode(worldspaceWidget, QIcon(":placeholder"), Mask_Pathgrid | Mask_Terrain | Mask_Reference,
            getTooltip(), parent)
        , mDragMode(DragMode_None)
        , mFromNode(0)
        , mSelectionMode(0)
    {
    }

    QString PathgridMode::getTooltip()
    {
        return QString(
            "Pathgrid editing"
            "<ul><li>Primary edit: Add node to scene</li>"
            "<li>Secondary edit: Connect selected nodes to node</li>"
            "<li>Primary drag: Move selected nodes</li>"
            "<li>Secondary drag: Connect one node to another</li>"
            "<li>Other operations may be done with the context menu</li>"
            "</ul><p>Note: Only a single cell's pathgrid may be edited at a time");
    }

    void PathgridMode::activate(CSVWidget::SceneToolbar* toolbar)
    {
        if (!mSelectionMode)
        {
            mSelectionMode = new PathgridSelectionMode(toolbar, getWorldspaceWidget());
        }

        EditMode::activate(toolbar);
        toolbar->addTool(mSelectionMode);
    }

    void PathgridMode::primaryEditPressed(const WorldspaceHitResult& hitResult)
    {
        // Get pathgrid cell
        Cell* cell = getWorldspaceWidget().getCell (hitResult.worldPos);
        if (cell)
        {
            // Add node
            QUndoStack& undoStack = getWorldspaceWidget().getDocument().getUndoStack();
            QString description = "Connect node to selected nodes";

            CSMWorld::CommandMacro macro(undoStack, description);
            cell->getPathgrid()->applyPoint(macro, hitResult.worldPos);
        }
    }

    void PathgridMode::secondaryEditPressed(const WorldspaceHitResult& hit)
    {
        if (hit.tag)
        {
            if (PathgridTag* tag = dynamic_cast<PathgridTag*>(hit.tag.get()))
            {
                if (tag->getPathgrid()->isSelected())
                {
                    unsigned short node = SceneUtil::getPathgridNode(static_cast<unsigned short>(hit.index0));

                    QUndoStack& undoStack = getWorldspaceWidget().getDocument().getUndoStack();
                    QString description = "Connect node to selected nodes";

                    CSMWorld::CommandMacro macro(undoStack, description);
                    tag->getPathgrid()->applyEdges(macro, node);
                }
            }
        }
    }

    void PathgridMode::primarySelectPressed(const WorldspaceHitResult& hit)
    {
        getWorldspaceWidget().clearSelection(Mask_Pathgrid);

        if (hit.tag)
        {
            if (PathgridTag* tag = dynamic_cast<PathgridTag*>(hit.tag.get()))
            {
                mLastId = tag->getPathgrid()->getId();
                unsigned short node = SceneUtil::getPathgridNode(static_cast<unsigned short>(hit.index0));
                tag->getPathgrid()->toggleSelected(node);
            }
        }
    }

    void PathgridMode::secondarySelectPressed(const WorldspaceHitResult& hit)
    {
        if (hit.tag)
        {
            if (PathgridTag* tag = dynamic_cast<PathgridTag*>(hit.tag.get()))
            {
                if (tag->getPathgrid()->getId() != mLastId)
                {
                    getWorldspaceWidget().clearSelection(Mask_Pathgrid);
                    mLastId = tag->getPathgrid()->getId();
                }

                unsigned short node = SceneUtil::getPathgridNode(static_cast<unsigned short>(hit.index0));
                tag->getPathgrid()->toggleSelected(node);

                return;
            }
        }

        getWorldspaceWidget().clearSelection(Mask_Pathgrid);
    }

    bool PathgridMode::primaryEditStartDrag(const QPoint& pos)
    {
        std::vector<osg::ref_ptr<TagBase> > selection = getWorldspaceWidget().getSelection (Mask_Pathgrid);

        if (!selection.empty())
        {
            mDragMode = DragMode_Move;
        }

        return true;
    }

    bool PathgridMode::secondaryEditStartDrag(const QPoint& pos)
    {
        WorldspaceHitResult hit = getWorldspaceWidget().mousePick (pos, getWorldspaceWidget().getInteractionMask());
        if (hit.tag)
        {
            if (PathgridTag* tag = dynamic_cast<PathgridTag*>(hit.tag.get()))
            {
                mDragMode = DragMode_Edge;
                mEdgeId = tag->getPathgrid()->getId();
                mFromNode = SceneUtil::getPathgridNode(static_cast<unsigned short>(hit.index0));

                tag->getPathgrid()->setupConnectionIndicator(mFromNode);
            }
        }

        return true;
    }

    void PathgridMode::drag(const QPoint& pos, int diffX, int diffY, double speedFactor)
    {
        std::vector<osg::ref_ptr<TagBase> > selection = getWorldspaceWidget().getSelection (Mask_Pathgrid);

        for (std::vector<osg::ref_ptr<TagBase> >::iterator it = selection.begin(); it != selection.end(); ++it)
        {
            if (PathgridTag* tag = dynamic_cast<PathgridTag*>(it->get()))
            {
                if (mDragMode == DragMode_Move)
                {
                    osg::Vec3d eye, center, up, offset;
                    getWorldspaceWidget().getCamera()->getViewMatrix().getLookAt (eye, center, up);

                    offset = (up * diffY * speedFactor) + (((center - eye) ^ up) * diffX * speedFactor);

                    tag->getPathgrid()->moveSelected(offset);
                }
                else if (mDragMode == DragMode_Edge)
                {
                    // TODO Add indicator, need raytrace
                }
            }
        }
    }

    void PathgridMode::dragCompleted(const QPoint& pos)
    {
        if (mDragMode == DragMode_Move)
        {
            std::vector<osg::ref_ptr<TagBase> > selection = getWorldspaceWidget().getSelection (Mask_Pathgrid);
            for (std::vector<osg::ref_ptr<TagBase> >::iterator it = selection.begin(); it != selection.end(); ++it)
            {
                if (PathgridTag* tag = dynamic_cast<PathgridTag*>(it->get()))
                {
                    QUndoStack& undoStack = getWorldspaceWidget().getDocument().getUndoStack();
                    QString description = "Move pathgrid node(s)";

                    CSMWorld::CommandMacro macro(undoStack, description);
                    tag->getPathgrid()->applyPosition(macro);
                }
            }
        }
        else if (mDragMode == DragMode_Edge)
        {
            WorldspaceHitResult hit = getWorldspaceWidget().mousePick(pos, getWorldspaceWidget().getInteractionMask());

            if (hit.tag)
            {
                if (PathgridTag* tag = dynamic_cast<PathgridTag*>(hit.tag.get()))
                {
                    if (tag->getPathgrid()->getId() == mEdgeId)
                    {
                        unsigned short toNode = SceneUtil::getPathgridNode(static_cast<unsigned short>(hit.index0));

                        QUndoStack& undoStack = getWorldspaceWidget().getDocument().getUndoStack();
                        QString description = "Add edge between nodes";

                        CSMWorld::CommandMacro macro(undoStack, description);
                        tag->getPathgrid()->applyEdge(macro, mFromNode, toNode);
                    }
                }
            }

            mEdgeId.clear();
            mFromNode = 0;
        }

        mDragMode = DragMode_None;
        getWorldspaceWidget().reset(Mask_Pathgrid);
    }

    void PathgridMode::dragAborted()
    {
        getWorldspaceWidget().reset(Mask_Pathgrid);
    }
}
