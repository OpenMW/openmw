#include "occlusionquerynode.hpp"

#include <OpenThreads/ScopedLock>
#include <osg/Timer>
#include <osg/Notify>
#include <osg/PolygonMode>
#include <osg/ColorMask>
#include <osg/PolygonOffset>
#include <osg/Depth>

#if OSG_VERSION_GREATER_OR_EQUAL(3,6,0)
#include <osg/ContextData>
#endif

#include "apps/openmw/mwrender/renderbin.hpp"

using namespace osg;

namespace SceneUtil
{

GLushort cubeindices[] = { 0, 1, 2, 3,  4, 5, 6, 7,
    0, 3, 6, 5,  2, 1, 4, 7,
    5, 4, 1, 0,  2, 7, 6, 3 };

osg::ref_ptr< osg::StateSet > StaticOcclusionQueryNode::OQDebugStateSet = 0;
osg::ref_ptr< osg::StateSet > StaticOcclusionQueryNode::OQStateSet = 0;
osg::Camera * StaticOcclusionQueryNode::defaultMainCamera = 0;

osg::StateSet* StaticOcclusionQueryNode::initMWOQState()
{
    if(OQStateSet.valid()) return OQStateSet;
    OQStateSet= new osg::StateSet;

    OQStateSet->setRenderBinDetails( MWRender::RenderBin_OcclusionQuery, "RenderBin", osg::StateSet::PROTECTED_RENDERBIN_DETAILS);

    OQStateSet->setMode( GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);
    OQStateSet->setTextureMode( 0, GL_TEXTURE_2D, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);
    OQStateSet->setMode( GL_CULL_FACE, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);

    osg::ColorMask* cm = new osg::ColorMask( false, false, false, false );
    OQStateSet->setAttributeAndModes( cm, osg::StateAttribute::ON |osg:: StateAttribute::PROTECTED);

    osg::Depth* d = new osg::Depth( osg::Depth::LESS, 0.f, 1.f, false );
    OQStateSet->setAttributeAndModes( d, osg::StateAttribute::ON | osg::StateAttribute::PROTECTED);

    return OQStateSet;
}


// Create and return a StateSet for rendering a debug representation of query geometry.
osg::StateSet* StaticOcclusionQueryNode::initMWOQDebugState()
{
    if(OQDebugStateSet.valid()) return OQDebugStateSet;
    OQDebugStateSet= new osg::StateSet;

    OQDebugStateSet->setMode( GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);
    OQDebugStateSet->setTextureMode( 0, GL_TEXTURE_2D, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);
    OQDebugStateSet->setMode( GL_CULL_FACE, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);

    osg::PolygonMode* pm = new osg::PolygonMode( osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::LINE );
    OQDebugStateSet->setAttributeAndModes( pm, osg::StateAttribute::ON | osg::StateAttribute::PROTECTED);

    return OQDebugStateSet;
}

bool StaticOcclusionQueryNode::getPassed( const Camera* camera, NodeVisitor& nv )
{
    if ( !_enabled )
    {
        // Queries are not enabled. The caller should be osgUtil::CullVisitor,
        //   return true to traverse the subgraphs.
        _passed = true;
        return _passed;
    }

    MWQueryGeometry* qg = static_cast< MWQueryGeometry* >( _queryGeode->getDrawable( 0 ) );

    if ( !_validQueryGeometry )
    {
        // There're cases that the occlusion test result has been retrieved
        // after the query geometry has been changed, it's the result of the
        // geometry before the change.
        qg->reset();

        // The box of the query geometry is invalid, return false to not traverse
        // the subgraphs.
        _passed = false;
        return _passed;
    }

    ///stat only main camera
    bool ret;
    bool & passed = ret;
    if(camera == _maincam)
        passed = _passed;

    //seems to bug (flickering) if OQN is not the last one in the tre
    if(_isgetpassedearlyexitenable)
    {
        // Two situations where we want to simply do a regular traversal:
        //  1) it's the first frame for this camera
        //  2) we haven't rendered for an abnormally long time (probably because we're an out-of-range LOD child)
        // In these cases, assume we're visible to avoid blinking.
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock( _frameCountMutex );
        const unsigned int& lastQueryFrame( _frameCountMap[ camera ] );
        if( //( lastQueryFrame == 0 ) ||
            ( (nv.getTraversalNumber() - lastQueryFrame) >  (_queryFrameCount+1 ) )
                )
        {
            passed = true;
            return passed;
        }
    }

    // Get the near plane for the upcoming distance calculation.
    osg::Matrix::value_type nearPlane;
    const osg::Matrix& proj( camera->getProjectionMatrix() );
    if( ( proj(3,3) != 1. ) || ( proj(2,3) != 0. ) || ( proj(1,3) != 0. ) || ( proj(0,3) != 0.) )
        nearPlane = proj(3,2) / (proj(2,2)-1.);  // frustum / perspective
    else
        nearPlane = (proj(3,2)+1.) / proj(2,2);  // ortho

    // If the distance from the near plane to the bounding sphere shell is positive, retrieve
    //   the results. Otherwise (near plane inside the BS shell) we are considered
    //   to have passed and don't need to retrieve the query.
    const osg::BoundingSphere& bs = getBound();
    osg::Matrix::value_type distanceToEyePoint = nv.getDistanceToEyePoint( bs._center, false );

    osg::Matrix::value_type distance = distanceToEyePoint - nearPlane - bs._radius;
    passed =  ( distance <= _securepopdistance );

    if (!passed)
    {
        MWQueryGeometry::QueryResult result = qg->getMWQueryResult( camera );
        if (!result.valid)
        {
           // The query hasn't finished yet and the result still
           // isn't available, return true to traverse the subgraphs.
           passed = true;
           return passed;
        }

        passed = ( result.numPixels >  _visThreshold );
    }

    return passed;
}
osg::BoundingSphere StaticOcclusionQueryNode::computeBound() const
{
    if(!_validQueryGeometry)
    {
            // Need to make this routine thread-safe. Typically called by the update
            //   Visitor, or just after the update traversal, but could be called by
            //   an application thread or by a non-osgViewer application.
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock( _computeBoundMutex )  ;

            // This is the logical place to put this code, but the method is const. Cast
            //   away constness to compute the bounding box and modify the query geometry.
            StaticOcclusionQueryNode* nonConstThis = const_cast<StaticOcclusionQueryNode*>( this );

            osg::ComputeBoundsVisitor cbv;
            nonConstThis->accept( cbv );
            osg::BoundingBox bb = cbv.getBoundingBox();

            const bool bbValid = bb.valid();
            _validQueryGeometry = bbValid;

            osg::Geometry* geom = static_cast<osg:: Geometry* >( nonConstThis->_queryGeode->getDrawable( 0 ) );
            osg::ref_ptr<osg::Vec3Array> vert = static_cast<osg::Vec3Array*>(geom->getVertexArray());
            // Having (0,0,0) as vertices for the case of the invalid query geometry
            // still isn't quite the right thing. But the query geometry is public
            // accessible and therefore a user might expect eight vertices, so
            // it seems safer to keep eight vertices in the geometry.
            Vec3Array::iterator itv = vert->begin();
            if (bbValid)
            {
                bb._max+=osg::Vec3(_margin,_margin,_margin);
                bb._min-=osg::Vec3(_margin,_margin,_margin);
                geom->setInitialBound(bb);
                (*itv++) = osg::Vec3( bb._min.x(), bb._min.y(), bb._min.z() );
                (*itv++) = osg::Vec3( bb._max.x(), bb._min.y(), bb._min.z() );
                (*itv++) = osg::Vec3( bb._max.x(), bb._min.y(), bb._max.z() );
                (*itv++) = osg::Vec3( bb._min.x(), bb._min.y(), bb._max.z() );
                (*itv++) = osg::Vec3( bb._max.x(), bb._max.y(), bb._min.z() );
                (*itv++) = osg::Vec3( bb._min.x(), bb._max.y(), bb._min.z() );
                (*itv++) = osg::Vec3( bb._min.x(), bb._max.y(), bb._max.z() );
                (*itv++) = osg::Vec3( bb._max.x(), bb._max.y(), bb._max.z() );
                vert->dirty();
            }

    }

    return Group::computeBound();
}


void StaticOcclusionQueryNode::createSupportNodes()
{
    setDataVariance(osg::Object::STATIC);

    osg::DrawElementsUShort * dr = new osg::DrawElementsUShort( osg::PrimitiveSet::QUADS, 24,  cubeindices );
    dr->setDataVariance(Object::STATIC);

    osg::Vec3Array * vert = new osg::Vec3Array(8);
    vert->setDataVariance(Object::STATIC);

    _validQueryGeometry = false;

    {
        // Add the test geometry Geode
        _queryGeode = new osg::Geode;
        _queryGeode->setName( "OQTest" );
        _queryGeode->setDataVariance( Object::STATIC );

        osg::ref_ptr<MWQueryGeometry > geom = new MWQueryGeometry( /*this,*/getName() );
        geom->setDataVariance( Object::STATIC );
        geom->setVertexArray(vert);
        geom->addPrimitiveSet(dr);

        _queryGeode->addDrawable( geom.get() );
    }

    {
        // Add a Geode that is a visual representation of the
        //   test geometry for debugging purposes
        _debugGeode = new osg::Geode;
        _debugGeode->setName( "Debug" );
        _debugGeode->setDataVariance( Object::STATIC );

        osg::ref_ptr<osg::Geometry> geom = new osg::Geometry;
        geom->setDataVariance( Object::STATIC );
        geom->setVertexArray(vert);
        geom->addPrimitiveSet(dr);

        osg::ref_ptr<osg::Vec4Array> ca = new osg::Vec4Array;
        ca->push_back( osg::Vec4( 1.f, 1.f, 1.f, 1.f ) );
        geom->setColorArray( ca.get(), osg::Array::BIND_OVERALL );

        _debugGeode->addDrawable( geom.get() );
    }

    // Creste state sets. Note that the osgOQ visitors (which place OQNs throughout
    //   the scene graph) create a single instance of these StateSets shared
    //   between all OQNs for efficiency.
    setQueryStateSet( initMWOQState() );
    setDebugStateSet( initMWOQDebugState() );
}


struct RetrieveQueriesCallback : public osg::Camera::DrawCallback
{
    typedef std::vector<osg::ref_ptr<SceneUtil::TestResult> > ResultsVector;
    ResultsVector _results;

    RetrieveQueriesCallback( osg::GLExtensions* ext=NULL )  :
        _extensionsFallback( ext )
    {
    }

    RetrieveQueriesCallback( const RetrieveQueriesCallback& rqc, const osg::CopyOp& ) :
        _extensionsFallback( rqc._extensionsFallback )
    {
    }

    META_Object( osgMWOQ, RetrieveQueriesCallback )

    virtual void operator() (const osg::Camera& camera) const
    {
        if (_results.empty())
            return;

        const osg::Timer& timer = *osg::Timer::instance();
        osg::Timer_t start_tick = timer.tick();
        double elapsedTime( 0. );
        int count( 0 );

        const osg::GLExtensions* ext=0;
        if (camera.getGraphicsContext())
        {
            // The typical path, for osgViewer-based applications or any
            //   app that has set up a valid GraphicsCOntext for the Camera.
            ext = camera.getGraphicsContext()->getState()->get<osg::GLExtensions>();
        }
        else
        {
            // No valid GraphicsContext in the Camera. This might happen in
            //   SceneView-based apps. Rely on the creating code to have passed
            //   in a valid GLExtensions pointer, and hope it's valid for any
            //   context that might be current.
            OSG_DEBUG << "osgOQ: RQCB: Using fallback path to obtain GLExtensions pointer." << std::endl;
            ext = _extensionsFallback;
            if (!ext)
            {
                OSG_FATAL << "osgOQ: RQCB: GLExtensions pointer fallback is NULL." << std::endl;
                return;
            }
        }

        ResultsVector::const_iterator it = _results.begin();
        while (it != _results.end())
        {
            SceneUtil::TestResult* tr = const_cast<SceneUtil::TestResult*>( (*it).get() );

            if (!tr->_active || !tr->_init)
            {
                // This test wasn't executed last frame. This is probably because
                //   a parent node failed the OQ test, this node is outside the
                //   view volume, or we didn't run the test because we had not
                //   exceeded visibleQueryFrameCount.
                // Do not obtain results from OpenGL.
                it++;
                continue;
            }

            OSG_DEBUG <<
                "osgOQ: RQCB: Retrieving..." << std::endl;

            GLint ready = 0;
            ext->glGetQueryObjectiv( tr->_id, GL_QUERY_RESULT_AVAILABLE, &ready );
            if (ready)
            {
                ext->glGetQueryObjectiv( tr->_id, GL_QUERY_RESULT, &(tr->_numPixels) );
                if (tr->_numPixels < 0)
                    OSG_WARN << "osgOQ: RQCB: " <<
                    "glGetQueryObjectiv returned negative value (" << tr->_numPixels << ")." << std::endl;

                // Either retrieve last frame's results, or ignore it because the
                //   camera is inside the view. In either case, _active is now false.
                tr->_active = false;

            }
            // else: query result not available yet, try again next frame

            it++;
            count++;
        }

        elapsedTime = timer.delta_s(start_tick,timer.tick());
        OSG_INFO << "osgOQ: RQCB: " << "Retrieved " << count <<
            " queries in " << elapsedTime << " seconds." << std::endl;
    }

    void reset()
    {
        for (ResultsVector::iterator it = _results.begin(); it != _results.end();)
        {
            if (!(*it)->_active || !(*it)->_init) // remove results that have already been retrieved or their query objects deleted.
                it = _results.erase(it);
            else
                ++it;
        }
    }

    void add( SceneUtil::TestResult* tr )
    {
        _results.push_back( tr );
    }

    osg::GLExtensions* _extensionsFallback;
};



// PreDraw callback; clears the list of Results from the PostDrawCallback (above).
struct ClearQueriesCallback : public osg::Camera::DrawCallback
{
    ClearQueriesCallback() : _rqcb( NULL ) {}
    ClearQueriesCallback( const ClearQueriesCallback& rhs, const osg::CopyOp& copyop) : osg::Camera::DrawCallback(rhs, copyop), _rqcb(rhs._rqcb) {}
    META_Object( osgMWOQ, ClearQueriesCallback )

    virtual void operator() (const osg::Camera&) const
    {
        if (!_rqcb)
        {
            OSG_FATAL << "osgOQ: CQCB: Invalid RQCB." << std::endl;
            return;
        }
        _rqcb->reset();
    }

    RetrieveQueriesCallback* _rqcb;
};


MWQueryGeometry::MWQueryGeometry(  const std::string& oqnName )
  : osg::QueryGeometry(oqnName)
{
    setUseDisplayList( false );
    setUseVertexBufferObjects( true );
}
MWQueryGeometry::~MWQueryGeometry()
{
    reset();
}

unsigned int
MWQueryGeometry::getNumPixels( const osg::Camera* cam )
{
    return getMWQueryResult(cam).numPixels;
}

#if OSG_VERSION_LESS_THAN(3,6,0)
typedef std::list< GLuint > QueryObjectList;
typedef osg::buffered_object< QueryObjectList > DeletedQueryObjectCache;

static OpenThreads::Mutex s_mutex_deletedQueryObjectCache;
static DeletedQueryObjectCache s_deletedQueryObjectCache;
#else
class QueryObjectManager : public GLObjectManager
{
public:
    QueryObjectManager(unsigned int contextID) : GLObjectManager("QueryObjectManager", contextID) {}

    virtual void deleteGLObject(GLuint globj)
    {
        const GLExtensions* extensions = GLExtensions::Get(_contextID,true);
        if (extensions->isOcclusionQuerySupported || extensions->isARBOcclusionQuerySupported) extensions->glDeleteQueries( 1L, &globj );
    }
};
#endif

void
MWQueryGeometry::deleteQueryObject( unsigned int contextID, GLuint handle )
{
#if OSG_VERSION_LESS_THAN(3,6,0)
    if (handle!=0)
      {
          OpenThreads::ScopedLock< OpenThreads::Mutex > lock( s_mutex_deletedQueryObjectCache );

          // insert the handle into the cache for the appropriate context.
          s_deletedQueryObjectCache[contextID].push_back( handle );
  }
#else
    osg::get<QueryObjectManager>(contextID)->scheduleGLObjectForDeletion(handle);
#endif
}


void
MWQueryGeometry::flushDeletedQueryObjects( unsigned int contextID, double currentTime, double& availableTime )
{
#if OSG_VERSION_LESS_THAN(3,6,0)
    // if no time available don't try to flush objects.
        if (availableTime<=0.0) return;

        const osg::Timer& timer = *osg::Timer::instance();
        osg::Timer_t start_tick = timer.tick();
        double elapsedTime = 0.0;

        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock(s_mutex_deletedQueryObjectCache);

            const osg::GLExtensions* extensions = osg::GLExtensions::Get( contextID, true );

            QueryObjectList& qol = s_deletedQueryObjectCache[contextID];

            for(QueryObjectList::iterator titr=qol.begin();
                titr!=qol.end() && elapsedTime<availableTime;
                )
            {
                extensions->glDeleteQueries( 1L, &(*titr ) );
                titr = qol.erase(titr);
                elapsedTime = timer.delta_s(start_tick,timer.tick());
            }
        }

    availableTime -= elapsedTime;
#else
    osg::get<QueryObjectManager>(contextID)->flushDeletedGLObjects(currentTime, availableTime);
#endif
}

void
MWQueryGeometry::discardDeletedQueryObjects( unsigned int contextID )
{
#if OSG_VERSION_LESS_THAN(3,6,0)
    OpenThreads::ScopedLock< OpenThreads::Mutex > lock( s_mutex_deletedQueryObjectCache );
       QueryObjectList& qol = s_deletedQueryObjectCache[ contextID ];
   qol.clear();
#else
   osg::get<QueryObjectManager>(contextID)->discardAllGLObjects();
#endif
}

void MWQueryGeometry::releaseGLObjects( osg::State* state ) const
{
    Geometry::releaseGLObjects(state);

    if (!state)
    {
        // delete all query IDs for all contexts.
        const_cast<MWQueryGeometry*>(this)->reset();
    }
    else
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock( _mapMutex );

        // Delete all query IDs for the specified context.
        unsigned int contextID = state->getContextID();
        ResultMap::iterator it = _mwresults.begin();
        while (it != _mwresults.end())
        {
            osg::ref_ptr<SceneUtil::TestResult> tr = it->second;
            if (tr->_contextID == contextID)
            {
#if OSG_VERSION_LESS_THAN(3,6,0)
                MWQueryGeometry::deleteQueryObject( contextID, tr->_id );
#else
                osg::get<QueryObjectManager>(contextID)->scheduleGLObjectForDeletion(tr->_id );
#endif
                tr->_init = false;
            }
            it++;
        }
    }
}
MWQueryGeometry::QueryResult MWQueryGeometry::getMWQueryResult( const osg::Camera* cam )
{
    osg::ref_ptr<SceneUtil::TestResult> tr;
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock( _mapMutex );
        tr =  _mwresults[ cam ];
        if (!tr.valid())
        {
            tr = new SceneUtil::TestResult;
            _mwresults[ cam ] = tr;
        }

    }
    return QueryResult(tr->_init && !tr->_active, tr->_numPixels);
}
void MWQueryGeometry::drawImplementation( osg::RenderInfo& renderInfo ) const
{
    unsigned int contextID = renderInfo.getState()->getContextID();
    osg::GLExtensions* ext = renderInfo.getState()->get<GLExtensions>();

    if (!ext->isARBOcclusionQuerySupported && !ext->isOcclusionQuerySupported)
        return;

    osg::Camera* cam = renderInfo.getCurrentCamera();

    // Add callbacks if necessary.
    if (!cam->getPostDrawCallback())
    {
        RetrieveQueriesCallback* rqcb = new RetrieveQueriesCallback( ext );
        cam->setPostDrawCallback( rqcb );

        ClearQueriesCallback* cqcb = new ClearQueriesCallback;
        cqcb->_rqcb = rqcb;
        rqcb->reset();
        cam->setPreDrawCallback( cqcb );
    }

    // Get TestResult from Camera map
    osg::ref_ptr<SceneUtil::TestResult> tr;
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock( _mapMutex );
        tr = ( _mwresults[ cam ] );
        if (!tr.valid())
        {
            tr = new SceneUtil::TestResult;
            _mwresults[ cam ] = tr;
        }
    }


    // Issue query
    if (!tr->_init)
    {
        ext->glGenQueries( 1, &(tr->_id) );
        tr->_contextID = contextID;
        tr->_init = true;
    }

    if (tr->_active)
    {
        // last query hasn't been retrieved yet
        return;
    }

    // Add TestResult to RQCB.
    RetrieveQueriesCallback* rqcb = dynamic_cast<
        RetrieveQueriesCallback* >( cam->getPostDrawCallback() );
    if (!rqcb)
    {
        OSG_FATAL << "osgOQ: QG: Invalid RQCB." << std::endl;
        return;
    }

    OSG_DEBUG <<
        "osgOQ: QG: Querying for: " << _oqnName << std::endl;

    ext->glBeginQuery( GL_SAMPLES_PASSED_ARB, tr->_id );
    osg::Geometry::drawImplementation( renderInfo );
    ext->glEndQuery( GL_SAMPLES_PASSED_ARB );
    tr->_active = true;
    rqcb->add( tr.get() );


    OSG_DEBUG <<
        "osgOQ: QG. OQNName: " << _oqnName <<
        ", Ctx: " << contextID <<
        ", ID: " << tr->_id << std::endl;
#ifdef _DEBUG
    {
        GLenum err;
        if ((err = glGetError()) != GL_NO_ERROR)
        {
            OSG_FATAL << "osgOQ: QG: OpenGL error: " << err << "." << std::endl;
        }
    }
#endif


}

void OctreeAddRemove::recursivCellAddStaticObject(osg::BoundingSphere&bs, StaticOcclusionQueryNode &parent, osg::Group *child, osg::BoundingSphere& childbs)
{
    osg::Vec3i index =osg::Vec3i(childbs.center()[0]<bs.center()[0]?0:1,
                                 childbs.center()[1]<bs.center()[1]?0:1,
                                 childbs.center()[2]<bs.center()[2]?0:1);
    unsigned int ind = index[0] + index[1]*2 + index[2]*4;
    osg::Vec3 indexf = osg::Vec3(index[0]*2-1, index[1]*2-1, index[2]*2-1);
    osg::BoundingSphere bsi;
    bsi.radius() = bs.radius() * 0.5f;
    bsi.center() = bs.center() + indexf * bsi.radius();
    osg::ref_ptr<StaticOcclusionQueryNode> qnode;
    osg::ref_ptr<osg::Group> target = parent.getChild(ind)->asGroup();

    osg::BoundingSphere bst (target->getBound());
    if( bst.valid()
        && bst.radius() > mSettings.minOQNSize
        && bsi.radius() > mSettings.minOQNSize
        && target->getNumChildren() > mSettings.maxDrawablePerOQN
      )
    {
        qnode = dynamic_cast<StaticOcclusionQueryNode*>(target.get());
        if(!qnode.valid())
        {
            OSG_INFO<<"new OcclusionQueryNode with radius "<<bs.radius()<<std::endl;
            qnode = new StaticOcclusionQueryNode;
            for(unsigned int i=0; i<8; ++i)
                qnode->addChild(new osg::Group);

            qnode->setQueryMargin(mSettings.querymargin);
            qnode->setVisibilityThreshold(mSettings.querypixelcount);
            qnode->setDebugDisplay(mSettings.debugDisplay);
            qnode->setQueryFrameCount(mSettings.queryframecount);
            qnode->setDistancePreventingPopin(mSettings.securepopdistance);

            for(unsigned int i=0; i<target->getNumChildren(); ++i)
            {
                osg::Group * childi = target->getChild(i)->asGroup();
                osg::BoundingSphere bschild (childi->getBound());
                recursivCellAddStaticObject(bsi, *qnode, childi, bschild);
            }
            parent.setChild(ind, qnode);
            //avoid flickering when OQN goes hierarchical
            if(mSettings.maxBVHOQLevelCount > 1)
            {
                parent.setEarlyExitOn(false);
                qnode->setEarlyExitOn(false);
            }

        }
        recursivCellAddStaticObject(bsi, *qnode, child, childbs);
        qnode->invalidateQueryGeometry();

        //disable high BVH queries level
        float powlev = float(1<<mSettings.maxBVHOQLevelCount);
        if(bsi.radius() > powlev*mSettings.minOQNSize)
        {
            OSG_INFO<<"masking high level OQN"<<std::endl;
            parent.getQueryGeometry()->setNodeMask(0);
            parent.getDebugGeometry()->setNodeMask(0);
            parent.setQueriesEnabled(false);
            qnode->getQueryGeometry()->setNodeMask(0);
            qnode->getDebugGeometry()->setNodeMask(0);
            qnode->setQueriesEnabled(false);
        }

    }
    else
    {
        target->addChild(child);
        parent.getQueryGeometry()->setNodeMask(_OQGmask);
        parent.getDebugGeometry()->setNodeMask(_OQGmask);
        parent.setQueriesEnabled(true);
    }
}

bool OctreeAddRemove::recursivCellRemoveStaticObject(osg::OcclusionQueryNode & parent, osg::Node * childtoremove)
{
    osg::Group * pchild; bool removed=false;
    for(unsigned int i=0; i< parent.getNumChildren(); ++i)
    {
        pchild = parent.getChild(i)->asGroup();
        if((removed = pchild->removeChild(childtoremove))) break;
    }

    if(removed){

        osg::OcclusionQueryNode* curpar = &parent;

        while(curpar &&curpar->getParent(0))
        {
            unsigned int capacity = 0;
            for(unsigned int i=0; i<8; ++i)
                capacity += parent.getChild(i)->asGroup()->getNumChildren();
            /// TODO check other criterion for parent collapse
            if(capacity==0){
                ///collapse parent
                OSG_WARN<<"collapse empty OQN"<<std::endl;
                osg::Group *paparent=curpar->getParent(0);
                paparent->setChild(paparent->getChildIndex(curpar), new osg::Group);
                curpar=dynamic_cast<osg::OcclusionQueryNode*>(paparent);
            }
            else break;
        }
        return true;
    }
    else
    {
        for(unsigned int i=0; i< parent.getNumChildren(); ++i)
        {
            osg::OcclusionQueryNode * child = dynamic_cast<osg::OcclusionQueryNode*>(parent.getChild(i));
            if(child && recursivCellRemoveStaticObject(*child, childtoremove))
                return true;
        }
    }
    return false;
}

}
