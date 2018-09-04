#include "sdlvideowrapper.hpp"

#include <components/debug/debuglog.hpp>

#include <osgViewer/Viewer>

#include <SDL_video.h>

namespace SDLUtil
{

    VideoWrapper::VideoWrapper(SDL_Window *window, osg::ref_ptr<osgViewer::Viewer> viewer)
        : mWindow(window)
        , mViewer(viewer)
        , mGamma(1.f)
        , mContrast(1.f)
        , mHasSetGammaContrast(false)
    {
        SDL_GetWindowGammaRamp(mWindow, mOldSystemGammaRamp, &mOldSystemGammaRamp[256], &mOldSystemGammaRamp[512]);
    }

    VideoWrapper::~VideoWrapper()
    {
        SDL_SetWindowFullscreen(mWindow, 0);

        // If user hasn't touched the defaults no need to restore
        if (mHasSetGammaContrast)
            SDL_SetWindowGammaRamp(mWindow, mOldSystemGammaRamp, &mOldSystemGammaRamp[256], &mOldSystemGammaRamp[512]);
    }

    void VideoWrapper::setSyncToVBlank(bool sync)
    {
        osgViewer::Viewer::Windows windows;
        mViewer->getWindows(windows);
        mViewer->stopThreading();
        for (osgViewer::Viewer::Windows::iterator it = windows.begin(); it != windows.end(); ++it)
        {
            osgViewer::GraphicsWindow* win = *it;
            win->setSyncToVBlank(sync);
        }
        mViewer->startThreading();
    }

    void VideoWrapper::setGammaContrast(float gamma, float contrast)
    {
        if (gamma == mGamma && contrast == mContrast)
            return;

        mGamma = gamma;
        mContrast = contrast;

        mHasSetGammaContrast = true;

        Uint16 red[256], green[256], blue[256];
        for (int i = 0; i < 256; i++)
        {
            float k = i/256.0f;
            k = (k - 0.5f) * contrast + 0.5f;
            k = pow(k, 1.f/gamma);
            k *= 256;
            float value = k*256;
            if (value > 65535)  value = 65535;
            else if (value < 0) value = 0;

            red[i] = green[i] = blue[i] = static_cast<Uint16>(value);
        }
        if (SDL_SetWindowGammaRamp(mWindow, red, green, blue) < 0)
            Log(Debug::Warning) << "Couldn't set gamma: " << SDL_GetError();
    }

    void VideoWrapper::setVideoMode(int width, int height, bool fullscreen, bool windowBorder)
    {
        SDL_SetWindowFullscreen(mWindow, 0);

        if (SDL_GetWindowFlags(mWindow) & SDL_WINDOW_MAXIMIZED)
            SDL_RestoreWindow(mWindow);

        if (fullscreen)
        {
            SDL_DisplayMode mode;
            SDL_GetWindowDisplayMode(mWindow, &mode);
            mode.w = width;
            mode.h = height;
            SDL_SetWindowDisplayMode(mWindow, &mode);
            SDL_SetWindowFullscreen(mWindow, fullscreen);
        }
        else
        {
            SDL_SetWindowSize(mWindow, width, height);
            SDL_SetWindowBordered(mWindow, windowBorder ? SDL_TRUE : SDL_FALSE);
        }
    }

}
