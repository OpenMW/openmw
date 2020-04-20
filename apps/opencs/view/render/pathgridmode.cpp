#include "pathgridmode.hpp"

#include <QMenu>
#include <QPoint>

#include <components/sceneutil/pathgridutil.hpp>

#include "../../model/prefs/state.hpp"

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
            "<ul><li>Press {scene-edit-primary} to add a node to the cursor location</li>"
            "<li>Press {scene-edit-secondary} to connect the selected nodes to the node beneath the cursor</li>"
            "<li>Press {scene-edit-primary} and drag to move selected nodes</li>"
            "<li>Press {scene-edit-secondary} and drag to connect one node to another</li>"
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

    void PathgridMode::deactivate(CSVWidget::SceneToolbar* toolbar)
    {
        if (mSelectionMode)
        {
            toolbar->removeTool (mSelectionMode);
            delete mSelectionMode;
            mSelectionMode = 0;
        }
    }

    void PathgridMode::primaryOpenPressed(const WorldspaceHitResult& hitResult)
    {
    }

    void PathgridMode::primaryEditPressed(const WorldspaceHitResult& hitResult)
    {
        if (CSMPrefs::get()["3D Scene Input"]["context-select"].isTrue() &&
            dynamic_cast<PathgridTag*>(hitResult.tag.get()))
        {
            primarySelectPressed(hitResult);
        }
        else if (Cell* cell = getWorldspaceWidget().getCell (hitResult.worldPos))
        {
            if (cell->getPathgrid())
            {
                // Add node
                QUndoStack& undoStack = getWorldspaceWidget().getDocument().getUndoStack();
                QString description = "Add node";

                CSMWorld::CommandMacro macro(undoStack, description);
                cell->getPathgrid()->applyPoint(macro, hitResult.worldPos);
            }
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

        if (CSMPrefs::get()["3D Scene Input"]["context-select"].isTrue())
        {
            WorldspaceHitResult hit = getWorldspaceWidget().mousePick (pos, getWorldspaceWidget().getInteractionMask());

            if (dynamic_cast<PathgridTag*>(hit.tag.get()))
            {
                primarySelectPressed(hit);
                selection = getWorldspaceWidget().getSelection (Mask_Pathgrid);
            }
        }

        if (!selection.empty())
        {
            mDragMode = DragMode_Move;
            return true;
        }

        return false;
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

                tag->getPathgrid()->setDragOrigin(mFromNode);
                return true;
            }
        }

        return false;
    }

    void PathgridMode::drag(const QPoint& pos, int diffX, int diffY, double speedFactor)
    {
        if (mDragMode == DragMode_Move)
        {
            std::vector<osg::ref_ptr<TagBase> > selection = getWorldspaceWidget().getSelection(Mask_Pathgrid);

            for (std::vector<osg::ref_ptr<TagBase> >::iterator it = selection.begin(); it != selection.end(); ++it)
            {
                if (PathgridTag* tag = dynamic_cast<PathgridTag*>(it->get()))
                {
                    osg::Vec3d eye, center, up, offset;
                    getWorldspaceWidget().getCamera()->getViewMatrix().getLookAt (eye, center, up);

                    offset = (up * diffY * speedFactor) + (((center - eye) ^ up) * diffX * speedFactor);

                    tag->getPathgrid()->moveSelected(offset);
                }
            }
        }
        else if (mDragMode == DragMode_Edge)
        {
            WorldspaceHitResult hit = getWorldspaceWidget().mousePick (pos, getWorldspaceWidget().getInteractionMask());

            Cell* cell = getWorldspaceWidget().getCell(hit.worldPos);
            if (cell && cell->getPathgrid())
            {
                PathgridTag* tag = 0;
                if (hit.tag && (tag = dynamic_cast<PathgridTag*>(hit.tag.get())) && tag->getPathgrid()->getId() == mEdgeId)
                {
                    unsigned short node = SceneUtil::getPathgridNode(static_cast<unsigned short>(hit.index0));
                    cell->getPathgrid()->setDragEndpoint(node);
                }
                else
                {
                    cell->getPathgrid()->setDragEndpoint(hit.worldPos);
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
