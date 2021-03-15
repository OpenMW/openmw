#ifndef OPENMW_COMPONENTS_SDLUTIL_GL4ES_INIT_H
#define OPENMW_COMPONENTS_SDLUTIL_GL4ES_INIT_H
#ifdef OPENMW_GL4ES_MANUAL_INIT
#include <SDL_video.h>

// Must be called once SDL video mode has been set,
// which creates a context.
//
// GL4ES can then query the context for features and extensions.
extern "C" void openmw_gl4es_init(SDL_Window *window);

#endif  // OPENMW_GL4ES_MANUAL_INIT
#endif  // OPENMW_COMPONENTS_SDLUTIL_GL4ES_INIT_H
