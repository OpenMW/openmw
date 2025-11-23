#include "myguirendermanager.hpp"

#include <MyGUI_Timer.h>

#include <osg/Drawable>
#include <osg/TexMat>
#include <osg/Texture2D>

#include <osgViewer/Viewer>

#include <osgGA/GUIEventHandler>

#include <components/resource/imagemanager.hpp>
#include <components/sceneutil/nodecallback.hpp>
#include <components/shader/shadermanager.hpp>

#include "myguitexture.hpp"

#define MYGUI_PLATFORM_LOG_SECTION "Platform"
#define MYGUI_PLATFORM_LOG(level, text) MYGUI_LOGGING(MYGUI_PLATFORM_LOG_SECTION, level, text)

#define MYGUI_PLATFORM_EXCEPT(dest)                                                                                    \
    do                                                                                                                 \
    {                                                                                                                  \
        MYGUI_PLATFORM_LOG(Critical, dest);                                                                            \
        std::ostringstream stream;                                                                                     \
        stream << dest << "\n";                                                                                        \
        MYGUI_BASE_EXCEPT(stream.str().c_str(), "MyGUI");                                                              \
    } while (0)

#define MYGUI_PLATFORM_ASSERT(exp, dest)                                                                               \
    do                                                                                                                 \
    {                                                                                                                  \
        if (!(exp))                                                                                                    \
        {                                                                                                              \
            MYGUI_PLATFORM_LOG(Critical, dest);                                                                        \
            std::ostringstream stream;                                                                                 \
            stream << dest << "\n";                                                                                    \
            MYGUI_BASE_EXCEPT(stream.str().c_str(), "MyGUI");                                                          \
        }                                                                                                              \
    } while (0)

namespace MyGUIPlatform
{

    class Drawable : public osg::Drawable
    {
        MyGUIPlatform::RenderManager* mParent;
        osg::ref_ptr<osg::StateSet> mStateSet;

    public:
        // Stage 0: update widget animations and controllers. Run during the Update traversal.
        class FrameUpdate : public SceneUtil::NodeCallback<FrameUpdate>
        {
        public:
            FrameUpdate()
                : mRenderManager(nullptr)
            {
            }

            void setRenderManager(MyGUIPlatform::RenderManager* renderManager) { mRenderManager = renderManager; }

            void operator()(osg::Node*, osg::NodeVisitor*) { mRenderManager->update(); }

        private:
            MyGUIPlatform::RenderManager* mRenderManager;
        };

        // Stage 1: collect draw calls. Run during the Cull traversal.
        class CollectDrawCalls : public SceneUtil::NodeCallback<CollectDrawCalls>
        {
        public:
            CollectDrawCalls()
                : mRenderManager(nullptr)
            {
            }

            void setRenderManager(MyGUIPlatform::RenderManager* renderManager) { mRenderManager = renderManager; }

            void operator()(osg::Node*, osg::NodeVisitor*) { mRenderManager->collectDrawCalls(); }

        private:
            MyGUIPlatform::RenderManager* mRenderManager;
        };

        // Stage 2: execute the draw calls. Run during the Draw traversal. May run in parallel with the update traversal
        // of the next frame.
        void drawImplementation(osg::RenderInfo& renderInfo) const override
        {
            osg::State* state = renderInfo.getState();

            state->pushStateSet(mStateSet);
            state->apply();

            state->disableAllVertexArrays();
            state->setClientActiveTextureUnit(0);
            glEnableClientState(GL_VERTEX_ARRAY);
            glEnableClientState(GL_TEXTURE_COORD_ARRAY);
            glEnableClientState(GL_COLOR_ARRAY);

            mReadFrom = (mReadFrom + 1) % sNumBuffers;
            const std::vector<Batch>& vec = mBatchVector[mReadFrom];
            for (std::vector<Batch>::const_iterator it = vec.begin(); it != vec.end(); ++it)
            {
                const Batch& batch = *it;
                osg::VertexBufferObject* vbo = batch.mVertexBuffer;

                if (batch.mStateSet)
                {
                    state->pushStateSet(batch.mStateSet);
                    state->apply();
                }

                // A GUI element without an associated texture would be extremely rare.
                // It is worth it to use a dummy 1x1 black texture sampler instead of either adding a conditional or
                // relinking shaders.
                osg::Texture2D* texture = batch.mTexture;
                if (texture)
                    state->applyTextureAttribute(0, texture);
                else
                    state->applyTextureAttribute(0, mDummyTexture);

                osg::GLBufferObject* bufferobject = state->isVertexBufferObjectSupported()
                    ? vbo->getOrCreateGLBufferObject(state->getContextID())
                    : nullptr;
                if (bufferobject)
                {
                    state->bindVertexBufferObject(bufferobject);

                    glVertexPointer(3, GL_FLOAT, sizeof(MyGUI::Vertex), reinterpret_cast<char*>(0));
                    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(MyGUI::Vertex), reinterpret_cast<char*>(12));
                    glTexCoordPointer(2, GL_FLOAT, sizeof(MyGUI::Vertex), reinterpret_cast<char*>(16));
                }
                else
                {
                    glVertexPointer(3, GL_FLOAT, sizeof(MyGUI::Vertex),
                        reinterpret_cast<const char*>(vbo->getArray(0)->getDataPointer()));
                    glColorPointer(4, GL_UNSIGNED_BYTE, sizeof(MyGUI::Vertex),
                        reinterpret_cast<const char*>(vbo->getArray(0)->getDataPointer()) + 12);
                    glTexCoordPointer(2, GL_FLOAT, sizeof(MyGUI::Vertex),
                        reinterpret_cast<const char*>(vbo->getArray(0)->getDataPointer()) + 16);
                }

                glDrawArrays(GL_TRIANGLES, 0, static_cast<GLsizei>(batch.mVertexCount));

                if (batch.mStateSet)
                {
                    state->popStateSet();
                    state->apply();
                }
            }

            glDisableClientState(GL_VERTEX_ARRAY);
            glDisableClientState(GL_TEXTURE_COORD_ARRAY);
            glDisableClientState(GL_COLOR_ARRAY);

            state->popStateSet();

            state->unbindVertexBufferObject();
            state->dirtyAllVertexArrays();
            state->disableAllVertexArrays();
        }

    public:
        Drawable(MyGUIPlatform::RenderManager* parent = nullptr)
            : mParent(parent)
            , mWriteTo(0)
            , mReadFrom(0)
        {
            setSupportsDisplayList(false);

            osg::ref_ptr<CollectDrawCalls> collectDrawCalls = new CollectDrawCalls;
            collectDrawCalls->setRenderManager(mParent);
            setCullCallback(collectDrawCalls);

            osg::ref_ptr<FrameUpdate> frameUpdate = new FrameUpdate;
            frameUpdate->setRenderManager(mParent);
            setUpdateCallback(frameUpdate);

            mStateSet = new osg::StateSet;
            mStateSet->setMode(GL_LIGHTING, osg::StateAttribute::OFF);
            mStateSet->setTextureMode(0, GL_TEXTURE_2D, osg::StateAttribute::ON);
            mStateSet->setMode(GL_DEPTH_TEST, osg::StateAttribute::OFF);
            mStateSet->setMode(GL_BLEND, osg::StateAttribute::ON);

            mDummyTexture = new osg::Texture2D;
            mDummyTexture->setWrap(osg::Texture::WRAP_S, osg::Texture::CLAMP_TO_EDGE);
            mDummyTexture->setWrap(osg::Texture::WRAP_T, osg::Texture::CLAMP_TO_EDGE);
            mDummyTexture->setInternalFormat(GL_RGB);
            mDummyTexture->setTextureSize(1, 1);

            // need to flip tex coords since MyGUI uses DirectX convention of top left image origin
            osg::Matrix flipMat;
            flipMat.preMultTranslate(osg::Vec3f(0, 1, 0));
            flipMat.preMultScale(osg::Vec3f(1, -1, 1));
            mStateSet->setTextureAttribute(0, new osg::TexMat(flipMat), osg::StateAttribute::ON);
        }
        Drawable(const Drawable& copy, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY)
            : osg::Drawable(copy, copyop)
            , mParent(copy.mParent)
            , mStateSet(copy.mStateSet)
            , mWriteTo(0)
            , mReadFrom(0)
            , mDummyTexture(copy.mDummyTexture)
        {
        }

        // Defines the necessary information for a draw call
        struct Batch
        {
            // May be empty
            osg::ref_ptr<osg::Texture2D> mTexture;

            osg::ref_ptr<osg::VertexBufferObject> mVertexBuffer;
            // need to hold on to this too as the mVertexBuffer does not hold a ref to its own array
            osg::ref_ptr<osg::Array> mArray;

            // optional
            osg::ref_ptr<osg::StateSet> mStateSet;

            size_t mVertexCount;
        };

        void addBatch(const Batch& batch) { mBatchVector[mWriteTo].push_back(batch); }

        void clear()
        {
            mWriteTo = (mWriteTo + 1) % sNumBuffers;
            mBatchVector[mWriteTo].clear();
        }

        osg::StateSet* getDrawableStateSet() { return mStateSet; }

        META_Object(osgMyGUI, Drawable)

    private:
        // 2 would be enough in most cases, use 4 to get stereo working
        static const int sNumBuffers = 4;

        // double buffering approach, to avoid the need for synchronization with the draw thread
        std::vector<Batch> mBatchVector[sNumBuffers];

        int mWriteTo;
        mutable int mReadFrom;

        osg::ref_ptr<osg::Texture2D> mDummyTexture;
    };

    class OSGVertexBuffer : public MyGUI::IVertexBuffer
    {
        osg::ref_ptr<osg::VertexBufferObject> mBuffer[2];
        osg::ref_ptr<osg::UByteArray> mVertexArray[2];

        size_t mNeedVertexCount;

        unsigned int mCurrentBuffer;
        bool mUsed; // has the mCurrentBuffer been submitted to the rendering thread

        void destroy();
        osg::UByteArray* create();

    public:
        OSGVertexBuffer();
        virtual ~OSGVertexBuffer() {}

        void markUsed();

        osg::Array* getVertexArray();
        osg::VertexBufferObject* getVertexBuffer();

        void setVertexCount(size_t count) override;
        size_t getVertexCount() const override;

        MyGUI::Vertex* lock() override;
        void unlock() override;
    };

    OSGVertexBuffer::OSGVertexBuffer()
        : mNeedVertexCount(0)
        , mCurrentBuffer(0)
        , mUsed(false)
    {
    }

    void OSGVertexBuffer::markUsed()
    {
        mUsed = true;
    }

    void OSGVertexBuffer::setVertexCount(size_t count)
    {
        if (count == mNeedVertexCount)
            return;

        mNeedVertexCount = count;
    }

    size_t OSGVertexBuffer::getVertexCount() const
    {
        return mNeedVertexCount;
    }

    MyGUI::Vertex* OSGVertexBuffer::lock()
    {
        if (mUsed)
        {
            mCurrentBuffer = (mCurrentBuffer + 1) % 2;
            mUsed = false;
        }
        osg::UByteArray* array = mVertexArray[mCurrentBuffer];
        if (!array)
        {
            array = create();
        }
        else if (array->size() != mNeedVertexCount * sizeof(MyGUI::Vertex))
        {
            array->resize(mNeedVertexCount * sizeof(MyGUI::Vertex));
        }

        return (MyGUI::Vertex*)&(*array)[0];
    }

    void OSGVertexBuffer::unlock()
    {
        mVertexArray[mCurrentBuffer]->dirty();
        mBuffer[mCurrentBuffer]->dirty();
    }

    osg::UByteArray* OSGVertexBuffer::create()
    {
        mVertexArray[mCurrentBuffer]
            = new osg::UByteArray(static_cast<unsigned int>(mNeedVertexCount * sizeof(MyGUI::Vertex)));

        mBuffer[mCurrentBuffer] = new osg::VertexBufferObject;
        mBuffer[mCurrentBuffer]->setDataVariance(osg::Object::DYNAMIC);
        mBuffer[mCurrentBuffer]->setUsage(GL_DYNAMIC_DRAW);
        // NB mBuffer does not own the array
        mBuffer[mCurrentBuffer]->setArray(0, mVertexArray[mCurrentBuffer].get());

        return mVertexArray[mCurrentBuffer];
    }

    osg::Array* OSGVertexBuffer::getVertexArray()
    {
        return mVertexArray[mCurrentBuffer];
    }

    osg::VertexBufferObject* OSGVertexBuffer::getVertexBuffer()
    {
        return mBuffer[mCurrentBuffer];
    }

    // ---------------------------------------------------------------------------

    RenderManager::RenderManager(
        osgViewer::Viewer* viewer, osg::Group* sceneroot, Resource::ImageManager* imageManager, float scalingFactor)
        : mViewer(viewer)
        , mSceneRoot(sceneroot)
        , mImageManager(imageManager)
        , mUpdate(false)
        , mIsInitialise(false)
        , mInvScalingFactor(1.f)
        , mInjectState(nullptr)
    {
        if (scalingFactor != 0.f)
            mInvScalingFactor = 1.f / scalingFactor;
    }

    RenderManager::~RenderManager()
    {
        MYGUI_PLATFORM_LOG(Info, "* Shutdown: " << getClassTypeName());

        if (mGuiRoot.valid())
            mSceneRoot->removeChild(mGuiRoot.get());
        mGuiRoot = nullptr;
        mSceneRoot = nullptr;
        mViewer = nullptr;

        MYGUI_PLATFORM_LOG(Info, getClassTypeName() << " successfully shutdown");
        mIsInitialise = false;
    }

    void RenderManager::initialise()
    {
        MYGUI_PLATFORM_ASSERT(!mIsInitialise, getClassTypeName() << " initialised twice");
        MYGUI_PLATFORM_LOG(Info, "* Initialise: " << getClassTypeName());

        mVertexFormat = MyGUI::VertexColourType::ColourABGR;

        mUpdate = false;

        mDrawable = new Drawable(this);

        osg::ref_ptr<osg::Camera> camera = new osg::Camera();
        camera->setReferenceFrame(osg::Transform::ABSOLUTE_RF);
        camera->setProjectionResizePolicy(osg::Camera::FIXED);
        camera->setProjectionMatrix(osg::Matrix::identity());
        camera->setViewMatrix(osg::Matrix::identity());
        camera->setRenderOrder(osg::Camera::POST_RENDER);
        camera->setClearMask(GL_NONE);
        mDrawable->setCullingActive(false);
        camera->addChild(mDrawable.get());

        mGuiRoot = camera;
        mSceneRoot->addChild(mGuiRoot.get());

        osg::ref_ptr<osg::Viewport> vp = mViewer->getCamera()->getViewport();
        setViewSize(static_cast<int>(vp->width()), static_cast<int>(vp->height()));

        MYGUI_PLATFORM_LOG(Info, getClassTypeName() << " successfully initialized");
        mIsInitialise = true;
    }

    void RenderManager::shutdown()
    {
        mGuiRoot->removeChildren(0, mGuiRoot->getNumChildren());
        mSceneRoot->removeChild(mGuiRoot);
    }

    void RenderManager::enableShaders(Shader::ShaderManager& shaderManager)
    {
        auto program = shaderManager.getProgram("gui");

        mDrawable->getDrawableStateSet()->setAttributeAndModes(program, osg::StateAttribute::ON);
        mDrawable->getDrawableStateSet()->addUniform(new osg::Uniform("diffuseMap", 0));
    }

    MyGUI::IVertexBuffer* RenderManager::createVertexBuffer()
    {
        return new OSGVertexBuffer();
    }

    void RenderManager::destroyVertexBuffer(MyGUI::IVertexBuffer* buffer)
    {
        delete buffer;
    }

    void RenderManager::begin()
    {
        mDrawable->clear();
        // variance will be recomputed based on textures being rendered in this frame
        mDrawable->setDataVariance(osg::Object::STATIC);
    }

    void RenderManager::doRender(MyGUI::IVertexBuffer* buffer, MyGUI::ITexture* texture, size_t count)
    {
        Drawable::Batch batch;
        batch.mVertexCount = count;
        batch.mVertexBuffer = static_cast<OSGVertexBuffer*>(buffer)->getVertexBuffer();
        batch.mArray = static_cast<OSGVertexBuffer*>(buffer)->getVertexArray();
        static_cast<OSGVertexBuffer*>(buffer)->markUsed();

        if (OSGTexture* osgtexture = static_cast<OSGTexture*>(texture))
        {
            batch.mTexture = osgtexture->getTexture();
            if (batch.mTexture->getDataVariance() == osg::Object::DYNAMIC)
                mDrawable->setDataVariance(osg::Object::DYNAMIC); // only for this frame, reset in begin()
            if (!mInjectState && osgtexture->getInjectState())
                batch.mStateSet = osgtexture->getInjectState();
        }
        if (mInjectState)
            batch.mStateSet = mInjectState;

        mDrawable->addBatch(batch);
    }

    void RenderManager::setInjectState(osg::StateSet* stateSet)
    {
        mInjectState = stateSet;
    }

    void RenderManager::end() {}

    void RenderManager::update()
    {
        static MyGUI::Timer timer;
        static unsigned long lastLime = timer.getMilliseconds();
        unsigned long nowTime = timer.getMilliseconds();
        unsigned long time = nowTime - lastLime;

        onFrameEvent(static_cast<float>(static_cast<double>(time) / 1000));

        lastLime = nowTime;
    }

    void RenderManager::collectDrawCalls()
    {
        begin();
        onRenderToTarget(this, mUpdate);
        end();

        mUpdate = false;
    }

    void RenderManager::setViewSize(int width, int height)
    {
        if (width < 1)
            width = 1;
        if (height < 1)
            height = 1;

        mGuiRoot->setViewport(0, 0, width, height);

        mViewSize.set(static_cast<int>(width * mInvScalingFactor), static_cast<int>(height * mInvScalingFactor));

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

    MyGUI::ITexture* RenderManager::createTexture(const std::string& name)
    {
        const auto it = mTextures.insert_or_assign(name, OSGTexture(name, mImageManager)).first;
        return &it->second;
    }

    void RenderManager::destroyTexture(MyGUI::ITexture* texture)
    {
        if (texture == nullptr)
            return;

        const auto item = mTextures.find(texture->getName());
        MYGUI_PLATFORM_ASSERT(item != mTextures.end(), "Texture '" << texture->getName() << "' not found");

        mTextures.erase(item);
    }

    MyGUI::ITexture* RenderManager::getTexture(const std::string& name)
    {
        if (name.empty())
            return nullptr;

        const auto item = mTextures.find(name);
        if (item == mTextures.end())
        {
            MyGUI::ITexture* tex = createTexture(name);
            tex->loadFromFile(name);
            return tex;
        }
        return &item->second;
    }

    bool RenderManager::checkTexture(MyGUI::ITexture* /*texture*/)
    {
        // We support external textures that aren't registered via this manager, so can't implement this method
        // sensibly.
        return true;
    }

    void RenderManager::registerShader(const std::string& /*shaderName*/, const std::string& /*vertexProgramFile*/,
        const std::string& /*fragmentProgramFile*/)
    {
        MYGUI_PLATFORM_LOG(Warning, "osgMyGUI::RenderManager::registerShader is not implemented");
    }

}
