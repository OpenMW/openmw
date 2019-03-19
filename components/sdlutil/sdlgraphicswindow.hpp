#ifndef OPENMW_COMPONENTS_SDLUTIL_SDLGRAPHICSWINDOW_H
#define OPENMW_COMPONENTS_SDLUTIL_SDLGRAPHICSWINDOW_H

#include <SDL_video.h>

#include <osgViewer/GraphicsWindow>

namespace SDLUtil
{

class GraphicsWindowSDL2 : public osgViewer::GraphicsWindow
{
    SDL_Window*     mWindow;
    SDL_GLContext   mContext;

    bool            mValid;
    bool            mRealized;
    bool            mOwnsWindow;

    void init();

    virtual ~GraphicsWindowSDL2();

public:
    GraphicsWindowSDL2(osg::GraphicsContext::Traits *traits);

    virtual bool isSameKindAs(const Object* object) const { return dynamic_cast<const GraphicsWindowSDL2*>(object)!=0; }
    virtual const char* libraryName() const { return "osgViewer"; }
    virtual const char* className() const { return "GraphicsWindowSDL2"; }

    virtual bool valid() const { return mValid; }

    /** Realise the GraphicsContext.*/
    virtual bool realizeImplementation();

    /** Return true if the graphics context has been realised and is ready to use.*/
    virtual bool isRealizedImplementation() const { return mRealized; }

    /** Close the graphics context.*/
    virtual void closeImplementation();

    /** Make this graphics context current.*/
    virtual bool makeCurrentImplementation();

    /** Release the graphics context.*/
    virtual bool releaseContextImplementation();

    /** Swap the front and back buffers.*/
    virtual void swapBuffersImplementation();

    /** Set sync-to-vblank. */
    virtual void setSyncToVBlank(bool on);

    /** Set Window decoration.*/
    virtual bool setWindowDecorationImplementation(bool flag);

    /** Raise specified window */
    virtual void raiseWindow();

    /** Set the window's position and size.*/
    virtual bool setWindowRectangleImplementation(int x, int y, int width, int height);

    /** Set the name of the window */
    virtual void setWindowName(const std::string &name);

    /** Set mouse cursor to a specific shape.*/
    virtual void setCursor(MouseCursor cursor);

    /** Get focus.*/
    virtual void grabFocus() {}

    /** Get focus on if the pointer is in this window.*/
    virtual void grabFocusIfPointerInWindow() {}

    /** WindowData is used to pass in the SDL2 window handle attached to the GraphicsContext::Traits structure. */
    struct WindowData : public osg::Referenced
    {
        WindowData(SDL_Window *window) : mWindow(window)
        { }

        SDL_Window *mWindow;
    };

private:
    void setSwapInterval(bool enable);
};

}

#endif /* OSGGRAPHICSWINDOW_H */
