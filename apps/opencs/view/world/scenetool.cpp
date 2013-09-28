
#include "scenetool.hpp"

CSVWorld::SceneTool::SceneTool (QWidget *parent) : QPushButton (parent)
{
    setSizePolicy (QSizePolicy (QSizePolicy::Fixed, QSizePolicy::Fixed));
    setFixedSize (48, 48);
}