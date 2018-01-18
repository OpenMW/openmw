#include "sdlcursormanager.hpp"

#include <cassert>
#include <stdexcept>
#include <iostream>
#include <cstdlib>

#include <SDL_mouse.h>
#include <SDL_endian.h>

#include <osg/GraphicsContext>
#include <osg/Geometry>
#include <osg/Texture2D>
#include <osg/TexMat>
#include <osgViewer/GraphicsWindow>

#include "imagetosurface.hpp"

#if defined(OSG_LIBRARY_STATIC) && !defined(ANDROID)
// Sets the default windowing system interface according to the OS.
// Necessary for OpenSceneGraph to do some things, like decompression.
USE_GRAPHICSWINDOW()
#endif

namespace
{

    class MyGraphicsContext {
        public:
            MyGraphicsContext(int w, int h)
            {
                osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
                traits->x = 0;
                traits->y = 0;
                traits->width = w;
                traits->height = h;
                traits->red = 8;
                traits->green = 8;
                traits->blue = 8;
                traits->alpha = 8;
                traits->windowDecoration = false;
                traits->doubleBuffer = false;
                traits->sharedContext = 0;
                traits->pbuffer = true;

                osg::GraphicsContext::ScreenIdentifier si;
                si.readDISPLAY();
                if (si.displayNum<0) si.displayNum = 0;
                traits->displayNum = si.displayNum;
                traits->screenNum = si.screenNum;
                traits->hostName = si.hostName;

                _gc = osg::GraphicsContext::createGraphicsContext(traits.get());

                if (!_gc)
                {
                    std::cerr << "Failed to create pbuffer, failing back to normal graphics window." << std::endl;

                    traits->pbuffer = false;
                    _gc = osg::GraphicsContext::createGraphicsContext(traits.get());
                    if (!_gc)
                        throw std::runtime_error("Failed to create graphics context for image decompression");
                }

                if (_gc.valid())
                {
                    _gc->realize();
                    _gc->makeCurrent();
                }
            }

            osg::ref_ptr<osg::GraphicsContext> getContext()
            {
                return _gc;
            }

            bool valid() const { return _gc.valid() && _gc->isRealized(); }

        private:
            osg::ref_ptr<osg::GraphicsContext> _gc;
    };

    osg::ref_ptr<osg::Image> decompress (osg::ref_ptr<osg::Image> source, float rotDegrees)
    {
        // TODO: use software decompression once S3TC patent expires

        int width = source->s();
        int height = source->t();

        MyGraphicsContext context(width, height);

        osg::ref_ptr<osg::State> state = context.getContext()->getState();

        osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
        texture->setImage(source);
        texture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_BORDER);
        texture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_BORDER);
        texture->setBorderColor(osg::Vec4f(0.f, 0.f, 0.f, 0.f));

        osg::ref_ptr<osg::TexMat> texmat = new osg::TexMat;
        osg::Matrix texRot (osg::Matrix::identity());
        float theta ( osg::DegreesToRadians(-rotDegrees) );
        float cosTheta = std::cos(theta);
        float sinTheta = std::sin(theta);

        texRot(0,0) = cosTheta;
        texRot(1,0) = -sinTheta;
        texRot(0,1) = sinTheta;
        texRot(1,1) = cosTheta;
        // Offset center of rotation to center of texture
        texRot(3,0) = 0.5f + ( (-0.5f * cosTheta) - (-0.5f * sinTheta) );
        texRot(3,1) = 0.5f + ( (-0.5f * sinTheta) + (-0.5f * cosTheta) );

        texmat->setMatrix(texRot);

        state->applyTextureAttribute(0, texmat);

        osg::ref_ptr<osg::RefMatrix> identity (new osg::RefMatrix(osg::Matrix::identity()));
        state->applyModelViewMatrix(identity);
        state->applyProjectionMatrix(identity);

        state->applyMode(GL_TEXTURE_2D, true);
        state->applyTextureAttribute(0, texture);

        osg::ref_ptr<osg::Image> resultImage = new osg::Image;
        resultImage->allocateImage(width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE);

        osg::RenderInfo renderInfo;
        renderInfo.setState(state);

        glViewport(0, 0, width, height);

        osg::ref_ptr<osg::Geometry> geom;

#if defined(__APPLE__)
        // Extra flip needed on OS X systems due to a driver bug
        const char* envval = getenv("OPENMW_CURSOR_WORKAROUND");
        bool workaround = !envval || envval == std::string("1");
        std::string vendorString = (const char*)glGetString(GL_VENDOR);
        if (!envval)
            workaround = vendorString.find("Intel") != std::string::npos || vendorString.find("ATI") != std::string::npos || vendorString.find("AMD") != std::string::npos;
        if (workaround)
            geom = osg::createTexturedQuadGeometry(osg::Vec3(-1,1,0), osg::Vec3(2,0,0), osg::Vec3(0,-2,0));
        else
#endif
        geom = osg::createTexturedQuadGeometry(osg::Vec3(-1,-1,0), osg::Vec3(2,0,0), osg::Vec3(0,2,0));

        geom->drawImplementation(renderInfo);

        glReadPixels(0, 0, width, height, GL_RGBA, GL_UNSIGNED_BYTE, resultImage->data());

        geom->releaseGLObjects();
        source->releaseGLObjects();
        texture->releaseGLObjects();

        return resultImage;
    }

}

namespace SDLUtil
{

    SDLCursorManager::SDLCursorManager() :
        mEnabled(false),
        mInitialized(false)
    {
    }

    SDLCursorManager::~SDLCursorManager()
    {
        CursorMap::const_iterator curs_iter = mCursorMap.begin();

        while(curs_iter != mCursorMap.end())
        {
            SDL_FreeCursor(curs_iter->second);
            ++curs_iter;
        }

        mCursorMap.clear();
    }

    void SDLCursorManager::setEnabled(bool enabled)
    {
        if(mInitialized && enabled == mEnabled)
            return;

        mInitialized = true;
        mEnabled = enabled;

        //turn on hardware cursors
        if(enabled)
        {
            _setGUICursor(mCurrentCursor);
        }
        //turn off hardware cursors
        else
        {
            SDL_ShowCursor(SDL_FALSE);
        }
    }

    void SDLCursorManager::cursorChanged(const std::string& name)
    {
        mCurrentCursor = name;
        _setGUICursor(name);
    }

    void SDLCursorManager::_setGUICursor(const std::string &name)
    {
        auto it = mCursorMap.find(name);
        if (it != mCursorMap.end())
            SDL_SetCursor(it->second);
    }

    void SDLCursorManager::createCursor(const std::string& name, int rotDegrees, osg::Image* image, Uint8 hotspot_x, Uint8 hotspot_y)
    {
#ifndef ANDROID
        _createCursorFromResource(name, rotDegrees, image, hotspot_x, hotspot_y);
#endif
    }

    void SDLCursorManager::_createCursorFromResource(const std::string& name, int rotDegrees, osg::Image* image, Uint8 hotspot_x, Uint8 hotspot_y)
    {
        osg::ref_ptr<osg::Image> decompressed;

        if (mCursorMap.find(name) != mCursorMap.end())
            return;

        try {
            decompressed = decompress(image, static_cast<float>(rotDegrees));
        } catch (std::exception& e) {
            std::cerr << e.what() << std::endl;
            std::cerr <<"Using default cursor."<<std::endl;
            return;
        }

        SDL_Surface* surf = SDLUtil::imageToSurface(decompressed, true);

        //set the cursor and store it for later
        SDL_Cursor* curs = SDL_CreateColorCursor(surf, hotspot_x, hotspot_y);
        mCursorMap.insert(CursorMap::value_type(std::string(name), curs));

        //clean up
        SDL_FreeSurface(surf);
    }

}
