#include "sdlcursormanager.hpp"

#include <memory>
#include <stdexcept>
#include <string>

#include <SDL_endian.h>
#include <SDL_error.h>
#include <SDL_hints.h>
#include <SDL_mouse.h>
#include <SDL_render.h>

#include <osg/Geometry>
#include <osg/GraphicsContext>
#include <osg/Texture2D>
#include <osg/Version>
#include <osgViewer/GraphicsWindow>

#include <components/debug/debuglog.hpp>

#include "imagetosurface.hpp"

#if defined(OSG_LIBRARY_STATIC) && (!defined(ANDROID) || OSG_VERSION_GREATER_THAN(3, 6, 5))
// Sets the default windowing system interface according to the OS.
// Necessary for OpenSceneGraph to do some things, like decompression.
USE_GRAPHICSWINDOW()
#endif

namespace SDLUtil
{

    SDLCursorManager::SDLCursorManager()
        : mEnabled(false)
        , mInitialized(false)
    {
    }

    SDLCursorManager::~SDLCursorManager()
    {
        CursorMap::const_iterator cursIter = mCursorMap.begin();

        while (cursIter != mCursorMap.end())
        {
            SDL_FreeCursor(cursIter->second);
            ++cursIter;
        }

        mCursorMap.clear();
    }

    void SDLCursorManager::setEnabled(bool enabled)
    {
        if (mInitialized && enabled == mEnabled)
            return;

        mInitialized = true;
        mEnabled = enabled;

        // turn on hardware cursors
        if (enabled)
        {
            _setGUICursor(mCurrentCursor);
        }
        // turn off hardware cursors
        else
        {
            SDL_ShowCursor(SDL_FALSE);
        }
    }

    void SDLCursorManager::cursorChanged(std::string_view name)
    {
        mCurrentCursor = name;
        _setGUICursor(mCurrentCursor);
    }

    void SDLCursorManager::_setGUICursor(std::string_view name)
    {
        auto it = mCursorMap.find(name);
        if (it == mCursorMap.end())
            it = mCursorMap.find("arrow");
        if (it != mCursorMap.end())
            SDL_SetCursor(it->second);
    }

    void SDLCursorManager::createCursor(std::string_view name, double rotDegrees, osg::Image* image, int hotspotX,
        int hotspotY, int cursorWidth, int cursorHeight)
    {
#ifndef ANDROID
        _createCursorFromResource(name, rotDegrees, image, hotspotX, hotspotY, cursorWidth, cursorHeight);
#endif
    }

    void SDLCursorManager::removeCursor(std::string_view name)
    {
        auto it = mCursorMap.find(name);
        if (it == mCursorMap.end())
            return;
        SDL_FreeCursor(it->second);
        mCursorMap.erase(it);
    }

    SDLUtil::SurfaceUniquePtr decompress(
        osg::ref_ptr<osg::Image> source, double rotDegrees, int cursorWidth, int cursorHeight)
    {
        int width = source->s();
        int height = source->t();
        bool useAlpha = source->isImageTranslucent();

        osg::ref_ptr<osg::Image> decompressedImage = new osg::Image;
        decompressedImage->setFileName(source->getFileName());
        decompressedImage->allocateImage(width, height, 1, useAlpha ? GL_RGBA : GL_RGB, GL_UNSIGNED_BYTE);
        for (int s = 0; s < width; ++s)
            for (int t = 0; t < height; ++t)
                decompressedImage->setColor(source->getColor(s, t, 0), s, t, 0);

        Uint32 redMask = 0x000000ff;
        Uint32 greenMask = 0x0000ff00;
        Uint32 blueMask = 0x00ff0000;
        Uint32 alphaMask = useAlpha ? 0xff000000 : 0;

        SDL_Surface* cursorSurface = SDL_CreateRGBSurfaceFrom(decompressedImage->data(), width, height,
            decompressedImage->getPixelSizeInBits(), decompressedImage->getRowSizeInBytes(), redMask, greenMask,
            blueMask, alphaMask);
        if (cursorSurface == nullptr)
            throw std::runtime_error(std::string("Failed to create cursor surface: ") + SDL_GetError());
        SDLUtil::SurfaceUniquePtr cursorSurfacePtr(cursorSurface, SDL_FreeSurface);

        SDLUtil::SurfaceUniquePtr targetSurface(
            SDL_CreateRGBSurface(0, cursorWidth, cursorHeight, 32, redMask, greenMask, blueMask, alphaMask),
            SDL_FreeSurface);
        if (!targetSurface)
            throw std::runtime_error(std::string("Failed to create cursor target surface: ") + SDL_GetError());
        std::unique_ptr<SDL_Renderer, decltype(&SDL_DestroyRenderer)> renderer(
            SDL_CreateSoftwareRenderer(targetSurface.get()), SDL_DestroyRenderer);
        if (!renderer)
            throw std::runtime_error(std::string("Failed to create cursor renderer: ") + SDL_GetError());

        if (SDL_RenderClear(renderer.get()) != 0)
            throw std::runtime_error(std::string("Failed to clear cursor renderer: ") + SDL_GetError());

        SDL_SetHint(SDL_HINT_RENDER_SCALE_QUALITY, "1");
        std::unique_ptr<SDL_Texture, decltype(&SDL_DestroyTexture)> cursorTexture(
            SDL_CreateTextureFromSurface(renderer.get(), cursorSurfacePtr.get()), SDL_DestroyTexture);
        if (!cursorTexture)
            throw std::runtime_error(std::string("Failed to create cursor texture: ") + SDL_GetError());

        if (SDL_RenderCopyEx(renderer.get(), cursorTexture.get(), nullptr, nullptr, -rotDegrees, nullptr, SDL_FLIP_NONE)
            != 0)
            throw std::runtime_error(std::string("Failed to render cursor texture: ") + SDL_GetError());

        return targetSurface;
    }

    void SDLCursorManager::_createCursorFromResource(std::string_view name, double rotDegrees, osg::Image* image,
        int hotspotX, int hotspotY, int cursorWidth, int cursorHeight)
    {
        if (mCursorMap.find(name) != mCursorMap.end())
            return;

        try
        {
            auto surface = decompress(image, rotDegrees, cursorWidth, cursorHeight);

            // set the cursor and store it for later
            SDL_Cursor* curs = SDL_CreateColorCursor(surface.get(), hotspotX, hotspotY);
            if (curs == nullptr)
                throw std::runtime_error(std::string("Failed to create cursor: ") + SDL_GetError());

            mCursorMap.emplace(name, curs);
        }
        catch (std::exception& e)
        {
            Log(Debug::Warning) << e.what();
            Log(Debug::Warning) << "Using default cursor.";
            return;
        }
    }

}
