#ifndef OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIRENDERMANAGER_H
#define OPENMW_COMPONENTS_MYGUIPLATFORM_MYGUIRENDERMANAGER_H

#include <MyGUI_RenderManager.h>

#include <osg/ref_ptr>

namespace Resource
{
    class ImageManager;
}

namespace osgViewer
{
    class Viewer;
}

namespace osg
{
    class Group;
    class Camera;
    class RenderInfo;
    class StateSet;
}

namespace osgMyGUI
{

class Drawable;

class RenderManager : public MyGUI::RenderManager, public MyGUI::IRenderTarget
{
    osg::ref_ptr<osgViewer::Viewer> mViewer;
    osg::ref_ptr<osg::Group> mSceneRoot;
    osg::ref_ptr<Drawable> mDrawable;
    Resource::ImageManager* mImageManager;

    MyGUI::IntSize mViewSize;
    bool mUpdate;
    MyGUI::VertexColourType mVertexFormat;
    MyGUI::RenderTargetInfo mInfo;

    typedef std::map<std::string, MyGUI::ITexture*> MapTexture;
    MapTexture mTextures;

    bool mIsInitialise;

    osg::ref_ptr<osg::Camera> mGuiRoot;

    float mInvScalingFactor;

    osg::StateSet* mInjectState;

    void destroyAllResources();

public:
    RenderManager(osgViewer::Viewer *viewer, osg::Group *sceneroot, Resource::ImageManager* imageManager, float scalingFactor);
    virtual ~RenderManager();

    void initialise();
    void shutdown();

    void setScalingFactor(float factor);

    static RenderManager& getInstance() { return *getInstancePtr(); }
    static RenderManager* getInstancePtr()
    { return static_cast<RenderManager*>(MyGUI::RenderManager::getInstancePtr()); }

    /** @see RenderManager::getViewSize */
    virtual const MyGUI::IntSize& getViewSize() const { return mViewSize; }

    /** @see RenderManager::getVertexFormat */
    virtual MyGUI::VertexColourType getVertexFormat() { return mVertexFormat; }

    /** @see RenderManager::isFormatSupported */
    virtual bool isFormatSupported(MyGUI::PixelFormat format, MyGUI::TextureUsage usage);

    /** @see RenderManager::createVertexBuffer */
    virtual MyGUI::IVertexBuffer* createVertexBuffer();
    /** @see RenderManager::destroyVertexBuffer */
    virtual void destroyVertexBuffer(MyGUI::IVertexBuffer *buffer);

    /** @see RenderManager::createTexture */
    virtual MyGUI::ITexture* createTexture(const std::string &name);
    /** @see RenderManager::destroyTexture */
    virtual void destroyTexture(MyGUI::ITexture* _texture);
    /** @see RenderManager::getTexture */
    virtual MyGUI::ITexture* getTexture(const std::string &name);

    // Called by the update traversal
    void update();

    // Called by the cull traversal
    /** @see IRenderTarget::begin */
    virtual void begin();
    /** @see IRenderTarget::end */
    virtual void end();
    /** @see IRenderTarget::doRender */
    virtual void doRender(MyGUI::IVertexBuffer *buffer, MyGUI::ITexture *texture, size_t count);

    /** specify a StateSet to inject for rendering. The StateSet will be used by future doRender calls until you reset it to nullptr again. */
    void setInjectState(osg::StateSet* stateSet);

    /** @see IRenderTarget::getInfo */
    virtual const MyGUI::RenderTargetInfo& getInfo() { return mInfo; }

    bool checkTexture(MyGUI::ITexture* _texture);

/*internal:*/

    void collectDrawCalls();
    void setViewSize(int width, int height);
};

}

#endif
