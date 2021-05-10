#ifndef CSV_RENDER_COMMANDS_HPP
#define CSV_RENDER_COMMANDS_HPP

#include <QUndoCommand>

namespace ESM
{
    struct Land;
}

namespace CSVRender
{
    class TerrainSelection;
    

    class DrawTerrainSelectionCommand : public QUndoCommand
    {
    private:
        TerrainSelection& mTerrainSelection;

    public:
        DrawTerrainSelectionCommand(TerrainSelection& terrainSelection, QUndoCommand* parent = nullptr);

        void redo() override;
        void undo() override;
    };
}

#endif
