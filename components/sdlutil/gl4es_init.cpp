// EGL does not work reliably for feature detection.
// Instead, we initialize gl4es manually.
#ifdef OPENMW_GL4ES_MANUAL_INIT
#include "gl4es_init.h"

// For glHint
#include <GL/gl.h>

extern "C" {

#include <gl4es/gl4esinit.h>
#include <gl4es/gl4eshint.h>

static SDL_Window *gWindow;

void openmw_gl4es_GetMainFBSize(int *width, int *height)
{
    SDL_GetWindowSize(gWindow, width, height);
}

void openmw_gl4es_init(SDL_Window *window)
{
    gWindow = window;
    set_getprocaddress(SDL_GL_GetProcAddress);
    set_getmainfbsize(openmw_gl4es_GetMainFBSize);
    initialize_gl4es();

    // merge glBegin/glEnd in beams and console
    glHint(GL_BEGINEND_HINT_GL4ES, 1);
    // dxt unpacked to 16-bit looks ugly
    glHint(GL_AVOID16BITS_HINT_GL4ES, 1);
}

}  // extern "C"

#endif  // OPENMW_GL4ES_MANUAL_INIT
