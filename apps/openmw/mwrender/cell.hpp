#ifndef GAME_RENDER_CELL_H
#define GAME_RENDER_CELL_H

namespace MWRender 
{
    class CellRender
    {
        public: 
    
            virtual ~CellRender() {};
    
            /// Make the cell visible. Load the cell if necessary.
            virtual void show() = 0;
            
            /// Remove the cell from rendering, but don't remove it from
            /// memory.
            virtual void hide() = 0; 
    };
}

#endif

