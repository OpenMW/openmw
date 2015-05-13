#include "sdlcursormanager.hpp"

#include <cassert>

#include <SDL_mouse.h>
#include <SDL_endian.h>

#include <osg/GraphicsContext>
#include <osg/Geometry>
#include <osg/Texture2D>

#include "imagetosurface.hpp"

namespace
{

    class MyGraphicsContext {
        public:
            MyGraphicsContext(int w, int h)
            {
                osg::ref_ptr<osg::GraphicsContext::Traits> traits = new osg::GraphicsContext::Traits;
                traits->x = 0;
                traits->y = 0;
                traits->width = 1;//w;
                traits->height = 1;//h;
                traits->windowDecoration = false;
                traits->doubleBuffer = false;
                traits->sharedContext = 0;
                traits->pbuffer = true;

                _gc = osg::GraphicsContext::createGraphicsContext(traits.get());

                if (!_gc)
                {
                    osg::notify(osg::NOTICE)<<"Failed to create pbuffer, failing back to normal graphics window."<<std::endl;

                    traits->pbuffer = false;
                    _gc = osg::GraphicsContext::createGraphicsContext(traits.get());
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

    osg::ref_ptr<osg::Image> decompress (osg::ref_ptr<osg::Image> source)
    {
        int width = source->s();
        int height = source->t();

        MyGraphicsContext context(width, height);

        osg::ref_ptr<osg::State> state = context.getContext()->getState();

        osg::ref_ptr<osg::Texture2D> texture = new osg::Texture2D;
        texture->setImage(source);

        state->applyTextureAttribute(0, texture);

        osg::ref_ptr<osg::Image> resultImage = new osg::Image;
        resultImage->allocateImage(width, height, 1, GL_RGBA, GL_UNSIGNED_BYTE);

        assert(resultImage->isDataContiguous());

        // FIXME: implement for GL ES (PBO & glMapBufferRange?)
        glGetTexImage(GL_TEXTURE_2D, 0, GL_RGBA, GL_UNSIGNED_BYTE, resultImage->data());

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

    bool SDLCursorManager::cursorChanged(const std::string& name)
    {
        mCurrentCursor = name;

        CursorMap::const_iterator curs_iter = mCursorMap.find(name);

        //we have this cursor
        if(curs_iter != mCursorMap.end())
        {
            _setGUICursor(name);

            return false;
        }
        else
        {
            //they should get back to us with more info
            return true;
        }
    }

    void SDLCursorManager::_setGUICursor(const std::string &name)
    {
        SDL_SetCursor(mCursorMap.find(name)->second);
    }

    void SDLCursorManager::receiveCursorInfo(const std::string& name, int rotDegrees, osg::Image* image, Uint8 size_x, Uint8 size_y, Uint8 hotspot_x, Uint8 hotspot_y)
    {
        _createCursorFromResource(name, rotDegrees, image, size_x, size_y, hotspot_x, hotspot_y);
    }

    void SDLCursorManager::_createCursorFromResource(const std::string& name, int rotDegrees, osg::Image* image, Uint8 size_x, Uint8 size_y, Uint8 hotspot_x, Uint8 hotspot_y)
    {
        if (mCursorMap.find(name) != mCursorMap.end())
            return;

        osg::ref_ptr<osg::Image> decompressed = decompress(image);

        // TODO: rotate

        SDL_Surface* surf = SDLUtil::imageToSurface(decompressed, false);

        //set the cursor and store it for later
        SDL_Cursor* curs = SDL_CreateColorCursor(surf, hotspot_x, hotspot_y);
        mCursorMap.insert(CursorMap::value_type(std::string(name), curs));

        //clean up
        SDL_FreeSurface(surf);

        _setGUICursor(name);
    }

}
