#include "myguirendermanager.hpp"

#include <stdexcept>

#include <MyGUI_Gui.h>
#include <MyGUI_Timer.h>

#include <osg/Drawable>
#include <osg/Geode>
#include <osg/PolygonMode>
#include <osg/BlendFunc>
#include <osg/Depth>
#include <osg/TexEnv>
#include <osg/Texture2D>

#include <osgViewer/Viewer>

#include <osgGA/GUIEventHandler>

#include <components/resource/texturemanager.hpp>

#include "myguitexture.hpp"

#define MYGUI_PLATFORM_LOG_SECTION "Platform"
#define MYGUI_PLATFORM_LOG(level, text) MYGUI_LOGGING(MYGUI_PLATFORM_LOG_SECTION, level, text)

#define MYGUI_PLATFORM_EXCEPT(dest) do { \
    MYGUI_PLATFORM_LOG(Critical, dest); \
    MYGUI_DBG_BREAK;\
    std::ostringstream stream; \
    stream << dest << "\n"; \
    MYGUI_BASE_EXCEPT(stream.str().c_str(), "MyGUI"); \
} while(0)

#define MYGUI_PLATFORM_ASSERT(exp, dest) do { \
    if ( ! (exp) ) \
    { \
        MYGUI_PLATFORM_LOG(Critical, dest); \
        MYGUI_DBG_BREAK;\
        std::ostringstream stream; \
        stream << dest << "\n"; \
        MYGUI_BASE_EXCEPT(stream.str().c_str(), "MyGUI"); \
    } \
} while(0)

namespace
{

// Proxy to forward a Drawable's draw call to RenderManager::drawFrame
class Renderable : public osg::Drawable {
    osgMyGUI::RenderManager *mParent;

    virtual void drawImplementation(osg::RenderInfo &renderInfo) const
    { mParent->drawFrame(renderInfo); }

public:
    Renderable(osgMyGUI::RenderManager *parent=nullptr) : mParent(parent) { }
    Renderable(const Renderable &copy, const osg::CopyOp &copyop=osg::CopyOp::SHALLOW_COPY)
        : osg::Drawable(copy, copyop)
        , mParent(copy.mParent)
    { }

    META_Object(osgMyGUI, Renderable)
};

// Proxy to forward an OSG resize event to RenderManager::setViewSize
class ResizeHandler : public osgGA::GUIEventHandler {
    osgMyGUI::RenderManager *mParent;

    virtual bool handle(const osgGA::GUIEventAdapter &ea, osgGA::GUIActionAdapter &aa)
    {
        if(ea.getEventType() == osgGA::GUIEventAdapter::RESIZE)
        {
            int width = ea.getWindowWidth();
            int height = ea.getWindowHeight();
            mParent->setViewSize(width, height);
        }
        return false;
    }

public:
    ResizeHandler(osgMyGUI::RenderManager *parent=nullptr) : mParent(parent) { }
    ResizeHandler(const ResizeHandler &copy, const osg::CopyOp &copyop=osg::CopyOp::SHALLOW_COPY)
        : osg::Object(copy, copyop), osgGA::GUIEventHandler(copy, copyop)
        , mParent(copy.mParent)
    { }

    META_Object(osgMyGUI, ResizeHandler)
};

}


namespace osgMyGUI
{

class OSGVertexBuffer : public MyGUI::IVertexBuffer
{
    osg::ref_ptr<osg::VertexBufferObject> mBuffer;
    osg::ref_ptr<osg::Vec3Array> mPositionArray;
    osg::ref_ptr<osg::Vec4ubArray> mColorArray;
    osg::ref_ptr<osg::Vec2Array> mTexCoordArray;
    std::vector<MyGUI::Vertex> mLockedData;

    size_t mNeedVertexCount;

public:
    OSGVertexBuffer();
    virtual ~OSGVertexBuffer();

    virtual void setVertexCount(size_t count);
    virtual size_t getVertexCount();

    virtual MyGUI::Vertex *lock();
    virtual void unlock();

/*internal:*/
    void destroy();
    void create();

    osg::VertexBufferObject *getBuffer() const { return mBuffer.get(); }
};

OSGVertexBuffer::OSGVertexBuffer()
  : mNeedVertexCount(0)
{
}

OSGVertexBuffer::~OSGVertexBuffer()
{
    destroy();
}

void OSGVertexBuffer::setVertexCount(size_t count)
{
    if(count == mNeedVertexCount)
        return;

    mNeedVertexCount = count;
    destroy();
    create();
}

size_t OSGVertexBuffer::getVertexCount()
{
    return mNeedVertexCount;
}

MyGUI::Vertex *OSGVertexBuffer::lock()
{
    MYGUI_PLATFORM_ASSERT(mBuffer.valid(), "Vertex buffer is not created");

    // NOTE: Unfortunately, MyGUI wants the VBO data to be interleaved as a
    // MyGUI::Vertex structure. However, OSG uses non-interleaved elements, so
    // we have to give back a "temporary" structure array then copy it to the
    // actual VBO arrays on unlock. This is extra unfortunate since the VBO may
    // be backed by VRAM, meaning we could have up to 3 copies of the data
    // (which we'll need to keep for VBOs that are continually updated).
    mLockedData.resize(mNeedVertexCount, MyGUI::Vertex());
    return mLockedData.data();
}

void OSGVertexBuffer::unlock()
{
    osg::Vec3 *vec = &mPositionArray->front();
    for (std::vector<MyGUI::Vertex>::const_iterator it = mLockedData.begin(); it != mLockedData.end(); ++it)
    {
        const MyGUI::Vertex& elem = *it;
        vec->set(elem.x, elem.y, elem.z);
        ++vec;
    }
    osg::Vec4ub *clr = &mColorArray->front();
    for (std::vector<MyGUI::Vertex>::const_iterator it = mLockedData.begin(); it != mLockedData.end(); ++it)
    {
        const MyGUI::Vertex& elem = *it;
        union {
            MyGUI::uint32 ui;
            unsigned char ub4[4];
        } val = { elem.colour };
        clr->set(val.ub4[0], val.ub4[1], val.ub4[2], val.ub4[3]);
        ++clr;
    }
    osg::Vec2 *crds = &mTexCoordArray->front();
    for (std::vector<MyGUI::Vertex>::const_iterator it = mLockedData.begin(); it != mLockedData.end(); ++it)
    {
        const MyGUI::Vertex& elem = *it;
        crds->set(elem.u, elem.v);
        ++crds;
    }

    mBuffer->dirty();
}

void OSGVertexBuffer::destroy()
{
    mBuffer = nullptr;
    mPositionArray = nullptr;
    mColorArray = nullptr;
    mTexCoordArray = nullptr;
    std::vector<MyGUI::Vertex>().swap(mLockedData);
}

void OSGVertexBuffer::create()
{
    MYGUI_PLATFORM_ASSERT(!mBuffer.valid(), "Vertex buffer already exist");

    mPositionArray = new osg::Vec3Array(mNeedVertexCount);
    mColorArray = new osg::Vec4ubArray(mNeedVertexCount);
    mTexCoordArray = new osg::Vec2Array(mNeedVertexCount);
    mColorArray->setNormalize(true);

    mBuffer = new osg::VertexBufferObject;
    mBuffer->setDataVariance(osg::Object::DYNAMIC);
    mBuffer->setUsage(GL_STREAM_DRAW);
    mBuffer->setArray(0, mPositionArray.get());
    mBuffer->setArray(1, mColorArray.get());
    mBuffer->setArray(2, mTexCoordArray.get());
}

// ---------------------------------------------------------------------------

RenderManager::RenderManager(osgViewer::Viewer *viewer, osg::Group *sceneroot, Resource::TextureManager* textureManager)
  : mViewer(viewer)
  , mSceneRoot(sceneroot)
  , mTextureManager(textureManager)
  , mUpdate(false)
  , mIsInitialise(false)
{
}

RenderManager::~RenderManager()
{
    MYGUI_PLATFORM_LOG(Info, "* Shutdown: "<<getClassTypeName());

    if(mGuiRoot.valid())
        mSceneRoot->removeChild(mGuiRoot.get());
    mGuiRoot = nullptr;
    mSceneRoot = nullptr;
    mViewer = nullptr;

    destroyAllResources();

    MYGUI_PLATFORM_LOG(Info, getClassTypeName()<<" successfully shutdown");
    mIsInitialise = false;
}


void RenderManager::initialise()
{
    MYGUI_PLATFORM_ASSERT(!mIsInitialise, getClassTypeName()<<" initialised twice");
    MYGUI_PLATFORM_LOG(Info, "* Initialise: "<<getClassTypeName());

    mVertexFormat = MyGUI::VertexColourType::ColourABGR;

    mUpdate = false;

    osg::ref_ptr<osg::Drawable> drawable = new Renderable(this);
    drawable->setSupportsDisplayList(false);
    drawable->setUseVertexBufferObjects(true);
    drawable->setDataVariance(osg::Object::DYNAMIC);

    osg::ref_ptr<osg::Geode> geode = new osg::Geode;
    geode->addDrawable(drawable.get());

    osg::ref_ptr<osg::Camera> camera = new osg::Camera();
    camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
    camera->setProjectionResizePolicy(osg::Camera::FIXED);
    camera->setProjectionMatrix(osg::Matrix::identity());
    camera->setViewMatrix(osg::Matrix::identity());
    camera->setRenderOrder(osg::Camera::POST_RENDER);
    camera->setClearMask(GL_NONE);
    osg::StateSet *state = new osg::StateSet;
    state->setTextureMode(0, GL_TEXTURE_GEN_S, osg::StateAttribute::OFF);
    state->setTextureMode(0, GL_TEXTURE_GEN_T, osg::StateAttribute::OFF);
    state->setTextureMode(0, GL_TEXTURE_GEN_R, osg::StateAttribute::OFF);
    state->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
    state->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
    state->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
    state->setMode(GL_LIGHT0, osg::StateAttribute::OFF);
    state->setMode(GL_BLEND, osg::StateAttribute::ON);
    state->setMode(GL_FOG, osg::StateAttribute::OFF);
    state->setTextureAttribute(0, new osg::TexEnv(osg::TexEnv::MODULATE));
    state->setAttribute(new osg::PolygonMode(osg::PolygonMode::FRONT, osg::PolygonMode::FILL));
    state->setAttribute(new osg::BlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA));
    state->setMode(GL_CULL_FACE, osg::StateAttribute::OFF);
    geode->setStateSet(state);
    geode->setCullingActive(false);
    camera->addChild(geode.get());

    mGuiRoot = camera;
    mSceneRoot->addChild(mGuiRoot.get());
    mViewer->addEventHandler(new ResizeHandler(this));

    osg::ref_ptr<osg::Viewport> vp = mViewer->getCamera()->getViewport();
    setViewSize(vp->width(), vp->height());

    MYGUI_PLATFORM_LOG(Info, getClassTypeName()<<" successfully initialized");
    mIsInitialise = true;
}

void RenderManager::shutdown()
{
}

MyGUI::IVertexBuffer* RenderManager::createVertexBuffer()
{
    return new OSGVertexBuffer();
}

void RenderManager::destroyVertexBuffer(MyGUI::IVertexBuffer *buffer)
{
    delete buffer;
}


void RenderManager::begin()
{
    osg::State *state = mRenderInfo->getState();
    state->disableAllVertexArrays();
}

void RenderManager::doRender(MyGUI::IVertexBuffer *buffer, MyGUI::ITexture *texture, size_t count)
{
    osg::State *state = mRenderInfo->getState();
    osg::VertexBufferObject *vbo = static_cast<OSGVertexBuffer*>(buffer)->getBuffer();
    MYGUI_PLATFORM_ASSERT(vbo, "Vertex buffer is not created");

    if(texture)
    {
        osg::Texture2D *tex = static_cast<OSGTexture*>(texture)->getTexture();
        MYGUI_PLATFORM_ASSERT(tex, "Texture is not created");
        state->applyTextureAttribute(0, tex);
    }

    state->setVertexPointer(vbo->getArray(0));
    state->setColorPointer(vbo->getArray(1));
    state->setTexCoordPointer(0, vbo->getArray(2));

    glDrawArrays(GL_TRIANGLES, 0, count);
}

void RenderManager::end()
{
    osg::State *state = mRenderInfo->getState();
    state->disableTexCoordPointer(0);
    state->disableColorPointer();
    state->disableVertexPointer();
    state->unbindVertexBufferObject();
}

void RenderManager::drawFrame(osg::RenderInfo &renderInfo)
{
    MyGUI::Gui *gui = MyGUI::Gui::getInstancePtr();
    if(gui == nullptr) return;

    mRenderInfo = &renderInfo;

    static MyGUI::Timer timer;
    static unsigned long last_time = timer.getMilliseconds();
    unsigned long now_time = timer.getMilliseconds();
    unsigned long time = now_time - last_time;

    onFrameEvent((float)((double)(time) / (double)1000));

    last_time = now_time;

    begin();
    onRenderToTarget(this, mUpdate);
    end();

    mUpdate = false;
}

void RenderManager::setViewSize(int width, int height)
{
    if(width < 1) width = 1;
    if(height < 1) height = 1;

    mGuiRoot->setViewport(0, 0, width, height);
    mViewSize.set(width, height);

    mInfo.maximumDepth = 1;
    mInfo.hOffset = 0;
    mInfo.vOffset = 0;
    mInfo.aspectCoef = float(mViewSize.height) / float(mViewSize.width);
    mInfo.pixScaleX = 1.0f / float(mViewSize.width);
    mInfo.pixScaleY = 1.0f / float(mViewSize.height);

    onResizeView(mViewSize);
    mUpdate = true;
}


bool RenderManager::isFormatSupported(MyGUI::PixelFormat /*format*/, MyGUI::TextureUsage /*usage*/)
{
    return true;
}

MyGUI::ITexture* RenderManager::createTexture(const std::string &name)
{
    MapTexture::const_iterator item = mTextures.find(name);
    MYGUI_PLATFORM_ASSERT(item == mTextures.end(), "Texture '"<<name<<"' already exist");

    OSGTexture* texture = new OSGTexture(name, mTextureManager);
    mTextures.insert(std::make_pair(name, texture));
    return texture;
}

void RenderManager::destroyTexture(MyGUI::ITexture *texture)
{
    if(texture == nullptr)
        return;

    MapTexture::iterator item = mTextures.find(texture->getName());
    MYGUI_PLATFORM_ASSERT(item != mTextures.end(), "Texture '"<<texture->getName()<<"' not found");

    mTextures.erase(item);
    delete texture;
}

MyGUI::ITexture* RenderManager::getTexture(const std::string &name)
{
    MapTexture::const_iterator item = mTextures.find(name);
    if(item == mTextures.end())
    {
        MyGUI::ITexture* tex = createTexture(name);
        tex->loadFromFile(name);
        return tex;
    }
    return item->second;
}

void RenderManager::destroyAllResources()
{
    for (MapTexture::iterator it = mTextures.begin(); it != mTextures.end(); ++it)
        delete it->second;
    mTextures.clear();
}

bool RenderManager::checkTexture(MyGUI::ITexture* _texture)
{
    for (MapTexture::const_iterator item = mTextures.begin(); item != mTextures.end(); ++item)
    {
        if (item->second == _texture)
            return true;
    }
    return false;
}

}
