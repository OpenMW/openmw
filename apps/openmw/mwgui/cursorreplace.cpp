#include "cursorreplace.hpp"

#include <boost/filesystem.hpp>
#include <openengine/ogre/imagerotate.hpp>

#include <OgreResourceGroupManager.h>
#include <OgreRoot.h>

using namespace MWGui;

CursorReplace::CursorReplace()
{
    OEngine::Render::ImageRotate::rotate("textures\\tx_cursormove.dds", "mwpointer_vresize.png", 90);
    OEngine::Render::ImageRotate::rotate("textures\\tx_cursormove.dds", "mwpointer_dresize1.png", -45);
    OEngine::Render::ImageRotate::rotate("textures\\tx_cursormove.dds", "mwpointer_dresize2.png", 45);
}
