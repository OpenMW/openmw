#ifndef OENGINE_OGRE_OSX_UTILS_H
#define OENGINE_OGRE_OSX_UTILS_H

#include <SDL_syswm.h>

namespace OEngine {
namespace Render {

extern unsigned long WindowContentViewHandle(SDL_SysWMinfo &info);

}
}

#endif
