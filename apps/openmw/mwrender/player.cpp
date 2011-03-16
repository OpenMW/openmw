
#include "player.hpp"

namespace MWRender
{
    Player::Player (Ogre::Camera *camera, const std::string& handle)
    : mCamera (camera), mHandle (handle)
    {}
}
