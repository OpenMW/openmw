#include "cursorreplace.hpp"

#include <boost/filesystem.hpp>
#include <openengine/ogre/imagerotate.hpp>
#include <openengine/ogre/atlas.hpp>

#include <OgreResourceGroupManager.h>
#include <OgreRoot.h>

using namespace MWGui;

CursorReplace::CursorReplace()
{
    OEngine::Render::ImageRotate::rotate("textures\\tx_cursormove.dds", "mwpointer_vresize.png", 90);
    OEngine::Render::ImageRotate::rotate("textures\\tx_cursormove.dds", "mwpointer_dresize1.png", -45);
    OEngine::Render::ImageRotate::rotate("textures\\tx_cursormove.dds", "mwpointer_dresize2.png", 45);

    OEngine::Render::Atlas::createFromFile("atlas1.cfg", "mwgui1", "textures\\");
    OEngine::Render::Atlas::createFromFile("mainmenu.cfg", "mwgui2", "textures\\");
}
