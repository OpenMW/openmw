
#include "worldspacewidget.hpp"

#include "../world/scenetoolmode.hpp"

CSVRender::WorldspaceWidget::WorldspaceWidget (QWidget *parent)
: SceneWidget (parent)
{}

void CSVRender::WorldspaceWidget::selectNavigationMode (const std::string& mode)
{
    if (mode=="1st")
        setNavigation (&m1st);
    else if (mode=="free")
        setNavigation (&mFree);
    else if (mode=="orbit")
        setNavigation (&mOrbit);
}

void CSVRender::WorldspaceWidget::selectDefaultNavigationMode()
{
    setNavigation (&m1st);
}

CSVWorld::SceneToolMode *CSVRender::WorldspaceWidget::makeNavigationSelector (
    CSVWorld::SceneToolbar *parent)
{
    CSVWorld::SceneToolMode *tool = new CSVWorld::SceneToolMode (parent);

    tool->addButton (":door.png", "1st"); /// \todo replace icons
    tool->addButton (":GMST.png", "free");
    tool->addButton (":Info.png", "orbit");

    connect (tool, SIGNAL (modeChanged (const std::string&)),
        this, SLOT (selectNavigationMode (const std::string&)));

    return tool;
}