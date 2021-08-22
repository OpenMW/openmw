#ifndef CSV_RENDER_COMMANDS_HPP
#define CSV_RENDER_COMMANDS_HPP

#include <QPointer>

#include <QUndoCommand>

#include "worldspacewidget.hpp"

namespace CSVRender
{
    class TerrainSelection;

    /*
        Current solution to force a redrawing of the terrain-selection grid
        when undoing/redoing changes in the editor.
        This only triggers a simple redraw of the grid, so only use it in
        conjunction with actual data changes which deform the grid.

        Please note that this command needs to be put onto the QUndoStack twice:
        at the start and at the end of the related terrain manipulation.
        This makes sure that the grid is always updated after all changes have
        been undone or redone -- but it also means that the selection is redrawn
        once at the beginning of either action. Future refinement may solve that.
    */
    class DrawTerrainSelectionCommand : public QUndoCommand
    {

    private:
        QPointer<WorldspaceWidget> mWorldspaceWidget;

    public:
        DrawTerrainSelectionCommand(WorldspaceWidget* worldspaceWidget, QUndoCommand* parent = nullptr);

        void redo() override;
        void undo() override;

        void tryUpdate();
    };
}

#endif
