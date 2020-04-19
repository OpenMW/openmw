#include "CullVisitorEx"
#include "RenderStageEx"

/// Needed for mixing osg rendering with Qt 2D drawing using QPainter...
/// See http://forum.openscenegraph.org/viewtopic.php?t=15627&view=previous

class RenderStageCacheEx : public osg::Object, public osg::Observer
{
public:

    typedef std::map<osgUtil::CullVisitor*, osg::ref_ptr<osgUtil::RenderStage> >
    RenderStageMap;

    RenderStageCacheEx() {}
    RenderStageCacheEx(const RenderStageCacheEx&, const osg::CopyOp&) {}
    virtual ~RenderStageCacheEx()
    {
        for(RenderStageMap::iterator itr = _renderStageMap.begin();
            itr != _renderStageMap.end();
            ++itr)
        {
            itr->first->removeObserver(this);
        }
    }

    META_Object(Ex, RenderStageCacheEx)

    virtual void objectDeleted(void* object)
    {
        osg::Referenced* ref = reinterpret_cast<osg::Referenced*>(object);
        osgUtil::CullVisitor* cv = dynamic_cast<osgUtil::CullVisitor*>(ref);

        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);

        RenderStageMap::iterator itr = _renderStageMap.find(cv);

        if(itr != _renderStageMap.end())
        {
            _renderStageMap.erase(itr);
        }
    }

    void setRenderStage(osgUtil::CullVisitor* cv, osgUtil::RenderStage* rs)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        RenderStageMap::iterator itr = _renderStageMap.find(cv);

        if(itr == _renderStageMap.end())
        {
            _renderStageMap[cv] = rs;
            cv->addObserver(this);
        }
        else
        {
            itr->second = rs;
        }

    }

    osgUtil::RenderStage* getRenderStage(osgUtil::CullVisitor* cv)
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock(_mutex);
        RenderStageMap::iterator itr = _renderStageMap.find(cv);

        if(itr != _renderStageMap.end())
        {
            return itr->second.get();
        }
        else
        {
            return 0;
        }
    }


    /** Resize any per context GLObject buffers to specified size. */
    virtual void resizeGLObjectBuffers(unsigned int maxSize)
    {
        for(RenderStageMap::const_iterator itr = _renderStageMap.begin();
            itr != _renderStageMap.end();
            ++itr)
        {
            itr->second->resizeGLObjectBuffers(maxSize);
        }
    }

    /** If State is non-zero, this function releases any associated OpenGL objects for
         the specified graphics context. Otherwise, releases OpenGL objexts
         for all graphics contexts. */
    virtual void releaseGLObjects(osg::State* state = 0) const
    {
        for(RenderStageMap::const_iterator itr = _renderStageMap.begin();
            itr != _renderStageMap.end();
            ++itr)
        {
            itr->second->releaseGLObjects(state);
        }
    }

    OpenThreads::Mutex  _mutex;
    RenderStageMap      _renderStageMap;
};

void CullVisitorEx::apply(osg::Camera& camera)
{

    // **************************************************************
    // Code from RenderStage class

    // push the node's state.
    osg::StateSet* node_state = camera.getStateSet();

    if(node_state) pushStateSet(node_state);

    //#define DEBUG_CULLSETTINGS

#ifdef DEBUG_CULLSETTINGS

    if(osg::isNotifyEnabled(osg::NOTICE))
    {
        OSG_NOTICE << std::endl << std::endl << "CullVisitor, before : ";
        write(osg::notify(osg::NOTICE));
    }

#endif

    // Save current cull settings
    CullSettings saved_cull_settings(*this);

#ifdef DEBUG_CULLSETTINGS

    if(osg::isNotifyEnabled(osg::NOTICE))
    {
        OSG_NOTICE << "CullVisitor, saved_cull_settings : ";
        saved_cull_settings.write(osg::notify(osg::NOTICE));
    }

#endif

#if 1
    // set cull settings from this Camera
    setCullSettings(camera);

#ifdef DEBUG_CULLSETTINGS
    OSG_NOTICE << "CullVisitor, after setCullSettings(camera) : ";
    write(osg::notify(osg::NOTICE));
#endif
    // inherit the settings from above
    inheritCullSettings(saved_cull_settings, camera.getInheritanceMask());

#ifdef DEBUG_CULLSETTINGS
    OSG_NOTICE << "CullVisitor, after inheritCullSettings(saved_cull_settings," <<
               camera.getInheritanceMask() << ") : ";
    write(osg::notify(osg::NOTICE));
#endif

#else
    // activate all active cull settings from this Camera
    inheritCullSettings(camera);
#endif

    // set the cull mask.
    unsigned int savedTraversalMask = getTraversalMask();
    bool mustSetCullMask = (camera.getInheritanceMask() &
                            osg::CullSettings::CULL_MASK) == 0;

    if(mustSetCullMask) setTraversalMask(camera.getCullMask());

    osg::RefMatrix& originalModelView = *getModelViewMatrix();

    osg::RefMatrix* projection = 0;
    osg::RefMatrix* modelview = 0;

    if(camera.getReferenceFrame() == osg::Transform::RELATIVE_RF)
    {
        if(camera.getTransformOrder() == osg::Camera::POST_MULTIPLY)
        {
            projection = createOrReuseMatrix(*getProjectionMatrix() *
                                             camera.getProjectionMatrix());
            modelview = createOrReuseMatrix(*getModelViewMatrix() * camera.getViewMatrix());
        }
        else // pre multiply
        {
            projection = createOrReuseMatrix(camera.getProjectionMatrix() *
                                             (*getProjectionMatrix()));
            modelview = createOrReuseMatrix(camera.getViewMatrix() *
                                            (*getModelViewMatrix()));
        }
    }
    else
    {
        // an absolute reference frame
        projection = createOrReuseMatrix(camera.getProjectionMatrix());
        modelview = createOrReuseMatrix(camera.getViewMatrix());
    }


    if(camera.getViewport()) pushViewport(camera.getViewport());

    // record previous near and far values.
    value_type previous_znear = _computed_znear;
    value_type previous_zfar = _computed_zfar;

    // take a copy of the current near plane candidates
    DistanceMatrixDrawableMap  previousNearPlaneCandidateMap;
    previousNearPlaneCandidateMap.swap(_nearPlaneCandidateMap);

    DistanceMatrixDrawableMap  previousFarPlaneCandidateMap;
    previousFarPlaneCandidateMap.swap(_farPlaneCandidateMap);

    _computed_znear = FLT_MAX;
    _computed_zfar = -FLT_MAX;

    pushProjectionMatrix(projection);
    pushModelViewMatrix(modelview, camera.getReferenceFrame());

    // **************************************************************
    // New code

    if(camera.getRenderOrder() == osg::Camera::NESTED_RENDER)
    {
        handle_cull_callbacks_and_traverse(camera);
    }
    else
    {
        osgUtil::RenderStage* prevRenderStage = getCurrentRenderBin()->getStage();
        osg::ref_ptr<RenderStageCacheEx> rsCache = dynamic_cast<RenderStageCacheEx*>
                                                   (camera.getRenderingCache());

        if(!rsCache)
        {
            rsCache = new RenderStageCacheEx();
            camera.setRenderingCache(rsCache);
        }

        osg::ref_ptr<osgUtil::RenderStage> rtts = rsCache->getRenderStage(this);

        if(!rtts)
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(*
                                                             (camera.getDataChangeMutex()));

            rtts = new RenderStageEx();
            rsCache->setRenderStage(this, rtts.get());

            rtts->setCamera(&camera);

            if(camera.getInheritanceMask() & DRAW_BUFFER)
            {
                // inherit draw buffer from above.
                rtts->setDrawBuffer(prevRenderStage->getDrawBuffer(),
                                    prevRenderStage->getDrawBufferApplyMask());
            }
            else
            {
                rtts->setDrawBuffer(camera.getDrawBuffer());
            }

            if(camera.getInheritanceMask() & READ_BUFFER)
            {
                // inherit read buffer from above.
                rtts->setReadBuffer(prevRenderStage->getReadBuffer(),
                                    prevRenderStage->getReadBufferApplyMask());
            }
            else
            {
                rtts->setReadBuffer(camera.getReadBuffer());
            }
        }
        else
        {
            // reusing render to texture stage, so need to reset it to empty it from previous frames contents.
            rtts->reset();
        }

        // **************************************************************
        // Code from RenderStage class

        // set up clear masks/values
        rtts->setClearDepth(camera.getClearDepth());
        rtts->setClearAccum(camera.getClearAccum());
        rtts->setClearStencil(camera.getClearStencil());
        rtts->setClearMask((camera.getInheritanceMask() & CLEAR_MASK) ?
                           prevRenderStage->getClearMask() : camera.getClearMask());
        rtts->setClearColor((camera.getInheritanceMask() & CLEAR_COLOR) ?
                            prevRenderStage->getClearColor() : camera.getClearColor());

        // set the color mask.
        osg::ColorMask* colorMask = camera.getColorMask() != 0 ? camera.getColorMask() :
                                    prevRenderStage->getColorMask();
        rtts->setColorMask(colorMask);

        // set up the viewport.
        osg::Viewport* viewport = camera.getViewport() != 0 ? camera.getViewport() :
                                  prevRenderStage->getViewport();
        rtts->setViewport(viewport);

        // set initial view matrix
        rtts->setInitialViewMatrix(modelview);

        // set up to charge the same PositionalStateContainer is the parent previous stage.
        osg::Matrix inheritedMVtolocalMV;
        inheritedMVtolocalMV.invert(originalModelView);
        inheritedMVtolocalMV.postMult(*getModelViewMatrix());
        rtts->setInheritedPositionalStateContainerMatrix(inheritedMVtolocalMV);
        rtts->setInheritedPositionalStateContainer(
            prevRenderStage->getPositionalStateContainer());

        // record the render bin, to be restored after creation
        // of the render to text
        osgUtil::RenderBin* previousRenderBin = getCurrentRenderBin();

        // set the current renderbin to be the newly created stage.
        setCurrentRenderBin(rtts.get());

        // traverse the subgraph
        {
            handle_cull_callbacks_and_traverse(camera);
        }

        // restore the previous renderbin.
        setCurrentRenderBin(previousRenderBin);


        if(rtts->getStateGraphList().size() == 0
           && rtts->getRenderBinList().size() == 0)
        {
            // getting to this point means that all the subgraph has been
            // culled by small feature culling or is beyond LOD ranges.
        }


        // and the render to texture stage to the current stages
        // dependency list.
        switch(camera.getRenderOrder())
        {
        case osg::Camera::PRE_RENDER:
            getCurrentRenderBin()->getStage()->addPreRenderStage(rtts.get(),
                                                                 camera.getRenderOrderNum());
            break;

        default:
            getCurrentRenderBin()->getStage()->addPostRenderStage(rtts.get(),
                                                                  camera.getRenderOrderNum());
            break;
        }
    }

    // **************************************************************
    // Code from RenderStage class
    // restore the previous model view matrix.
    popModelViewMatrix();

    // restore the previous model view matrix.
    popProjectionMatrix();


    // restore the original near and far values
    _computed_znear = previous_znear;
    _computed_zfar = previous_zfar;

    // swap back the near plane candidates
    previousNearPlaneCandidateMap.swap(_nearPlaneCandidateMap);
    previousFarPlaneCandidateMap.swap(_farPlaneCandidateMap);


    if(camera.getViewport()) popViewport();

    // restore the previous traversal mask settings
    if(mustSetCullMask) setTraversalMask(savedTraversalMask);

    // restore the previous cull settings
    setCullSettings(saved_cull_settings);

    // pop the node's state off the render graph stack.
    if(node_state) popStateSet();
}
