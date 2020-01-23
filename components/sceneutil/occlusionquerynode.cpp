#include "occlusionquerynode.hpp"

#include <OpenThreads/ScopedLock>
#include <osg/Timer>
#include <osg/Notify>
#include <osg/PolygonMode>
#include <osg/ColorMask>
#include <osg/PolygonOffset>
#include <osg/Depth>
#include <osgUtil/CullVisitor>

#if OSG_VERSION_GREATER_OR_EQUAL(3,6,0)
#include <osg/ContextData>
#endif

///test result initialization hacks
#define VISIBLE_AS_DEFAULT 1
//#define VISIBLE_AS_DEFAULT_ONLY_FOR_GUESS 1

//ensure minimum popin for in frustum occluded but drastically increase OQ count per frame
//#define PROVOK_OQ_4_PREVIOUSLY_OCCLUDED 1

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

    OQStateSet->setMode( GL_LIGHTING, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);
    OQStateSet->setTextureMode( 0, GL_TEXTURE_2D, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);
    OQStateSet->setMode( GL_CULL_FACE, osg::StateAttribute::OFF | osg::StateAttribute::PROTECTED);

    osg::ColorMask* cm = new osg::ColorMask( false, false, false, false );
    OQStateSet->setAttributeAndModes( cm, osg::StateAttribute::ON |osg:: StateAttribute::PROTECTED);

    osg::Depth* d = new osg::Depth( osg::Depth::LESS, 0.f, 1.f, false );
    OQStateSet->setAttributeAndModes( d, osg::StateAttribute::ON | osg::StateAttribute::PROTECTED);

    osg::PolygonMode* pm = new osg::PolygonMode( osg::PolygonMode::FRONT_AND_BACK, osg::PolygonMode::FILL );
    OQStateSet->setAttributeAndModes( pm, osg::StateAttribute::ON | osg::StateAttribute::PROTECTED);

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

void StaticOcclusionQueryNode::pullUpVisibility( const osg::Camera* cam, unsigned int numPix)
{
    StaticOcclusionQueryNode *parent = this;

    MWQueryGeometry* qg = parent->getQueryGeometry();
    while( qg->getLastQueryNumPixels(cam) == 0)
    {
        qg->forceQueryResult(cam, numPix);
        parent = dynamic_cast< StaticOcclusionQueryNode* >( parent->getParent(0) );
        if(!parent) return;
        qg = parent->getQueryGeometry();
    }
}

void StaticOcclusionQueryNode::pullDownVisibility( const osg::Camera* cam, unsigned int numPix)
{
    StaticOcclusionQueryNode *child;
    unsigned int numch = getNumChildren();
    for(unsigned int i=0; i<numch; ++i)
    {
        child = dynamic_cast<StaticOcclusionQueryNode*>(getChild(i));
        if(!child) return;

        MWQueryGeometry* qg = child->getQueryGeometry();;
        qg->forceQueryResult(cam, numPix);
        child->pullDownVisibility(cam, numPix);
    }
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

    if ( !isQueryGeometryValid() )
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

    unsigned int traversalNumber = nv.getTraversalNumber();
    bool wasVisible, wasTested;

    StaticOcclusionQueryNode* isnotLeaf = dynamic_cast<StaticOcclusionQueryNode*>(getChild(0));
    MWQueryGeometry::QueryResult result ;
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock( _frameCountMutex );
        unsigned int& lastQueryFrame( _frameCountMap[ camera ] );
        unsigned int& lasttestframe( _lastframes[ camera ] );
        result = qg->getMWQueryResult( camera );

        wasTested =  lasttestframe+1 >= traversalNumber;
        wasVisible = result.lastnumPixels>0 && wasTested;

        if( !wasTested )
        {
#if 0
            /// force entering frustum to be visible (huge oscillations on far plane narrowing yielding to perf loss)
            lastQueryFrame = traversalNumber;
            wasVisible = true;
            qg->forceQueryResult(camera, 1000);
            lasttestframe = traversalNumber;
            passed = true; return passed;
#else
            /// only provoke query for entering frustum
            if(isnotLeaf)
            {
                qg->forceQueryResult(camera, 1000);
                passed = true; return passed;
            }
            lastQueryFrame = 0;
            wasVisible = true;
#endif
        }


        lasttestframe = traversalNumber;
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
    const osg::BoundingSphere& bs = qg->getBound();
    osg::Matrix::value_type distanceToEyePoint = nv.getDistanceToEyePoint(bs._center, false);

    osg::Matrix::value_type distance = distanceToEyePoint - nearPlane - bs._radius;
    bool insecurearea =  ( distance <= _securepopdistance );

    if (!insecurearea)
    {
        if (result.valid)
        {
            passed = ( result.numPixels >  _visThreshold );
            if(isnotLeaf) if(!passed)  pullDownVisibility(camera, 0);

#ifdef PROVOK_OQ_4_PREVIOUSLY_OCCLUDED
        if(passed)
#endif
        return passed;
        }
    }
    else
    {
        /// force results at safe distance
        //TOFIX: a bit overkilling (2 mutices locks)
        {
            OpenThreads::ScopedLock<OpenThreads::Mutex> lock( _frameCountMutex );
            unsigned int& lastQueryFrame( _frameCountMap[ camera ] );
            lastQueryFrame = traversalNumber;
        }
        qg->forceQueryResult(camera, 1000);
        if(isnotLeaf)  pullDownVisibility(camera, 1000);
    }

    passed = insecurearea || wasVisible;
    if(isnotLeaf) if(!passed)  pullDownVisibility(camera, 0);
    return passed;
}


void StaticOcclusionQueryNode::resetStaticQueryGeometry()
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

    _queryGeometryState = bbValid ? USER_DEFINED : INVALID;

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

void StaticOcclusionQueryNode::createSupportNodes()
{
    setDataVariance(osg::Object::DYNAMIC);

    osg::DrawElementsUShort * dr = new osg::DrawElementsUShort( osg::PrimitiveSet::QUADS, 24,  cubeindices );
    dr->setDataVariance(Object::STATIC);

    osg::Vec3Array * vert = new osg::Vec3Array(8);
    vert->setDataVariance(Object::STATIC);

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

                tr->_lastnumPixels = tr->_numPixels;
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

unsigned int MWQueryGeometry::getNumPixels( const osg::Camera* cam ) const
{
    return getMWQueryResult(cam).numPixels;
}

unsigned int MWQueryGeometry::getLastQueryNumPixels( const osg::Camera* cam ) const
{
    return getMWQueryResult(cam).lastnumPixels;
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
MWQueryGeometry::QueryResult MWQueryGeometry::getMWQueryResult( const osg::Camera* cam ) const
{
    osg::ref_ptr<SceneUtil::TestResult> tr;
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock( _mapMutex );
        tr =  _mwresults[ cam ];
        if (!tr.valid())
        {
            tr = new SceneUtil::TestResult;
#ifdef VISIBLE_AS_DEFAULT
#ifndef VISIBLE_AS_DEFAULT_ONLY_FOR_GUESS
            tr->_numPixels =
#endif
            tr->_lastnumPixels = 1000;
#endif
            _mwresults[ cam ] = tr;
        }

    }
    return QueryResult((tr->_init && !tr->_active ), tr->_numPixels, tr->_lastnumPixels);
}

void MWQueryGeometry::forceQueryResult( const osg::Camera* cam, unsigned int numPixels) const
{
    osg::ref_ptr<SceneUtil::TestResult> tr;
    {
        OpenThreads::ScopedLock<OpenThreads::Mutex> lock( _mapMutex );
        tr =  _mwresults[ cam ];
        if (!tr.valid())
        {
            tr = new SceneUtil::TestResult();

#ifdef VISIBLE_AS_DEFAULT
#ifndef VISIBLE_AS_DEFAULT_ONLY_FOR_GUESS
            tr->_numPixels =
#endif
            tr->_lastnumPixels = 1000;
#endif
            _mwresults[ cam ] = tr;
        }
        tr->_lastnumPixels = numPixels;
        tr->_numPixels = numPixels;
    }
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
#ifdef VISIBLE_AS_DEFAULT
#ifndef VISIBLE_AS_DEFAULT_ONLY_FOR_GUESS
            tr->_numPixels =
#endif
            tr->_lastnumPixels = 1000;
#endif
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

    tr->_lastnumPixels = tr->_numPixels;
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

/// Avoid costy call to getBounds on OQN when bounds invalidated (deffered to further traversal)
/// but don't use cached bounds ...
class OQGetBoundsVisitor : public osg::NodeVisitor
{
public:
    OQGetBoundsVisitor(TraversalMode traversalMode = TRAVERSE_ALL_CHILDREN): osg::NodeVisitor(traversalMode) {}

    void apply(osg::Transform& transform)
    {
        osg::Matrix matrix;
        if (!_matrixStack.empty()) matrix = _matrixStack.back();

        transform.computeLocalToWorldMatrix(matrix,this);

        pushMatrix(matrix);

        traverse(transform);

        popMatrix();
    }
    void applyBoundingSphere(const osg::BoundingSphere& bbox)
    {
        if (_matrixStack.empty()) _bb.expandBy(bbox);
        else if (bbox.valid())
        {
            const osg::Matrix& matrix = _matrixStack.back();
            osg::BoundingSphere bs;
            bs.center() = bbox._center* matrix;
            bs.radius() = bbox.radius();
            _bb.expandBy(bs);
        }
    }
    void apply(osg::Drawable& drawable)
    {
        applyBoundingSphere(drawable.getBound());
    }

    inline void pushMatrix(osg::Matrix& matrix) { _matrixStack.push_back(matrix); }

    inline void popMatrix() { _matrixStack.pop_back(); }

    const osg::BoundingSphere& getBoundingSphere() { return _bb; }

    typedef std::vector<osg::Matrix> MatrixStack;

    const MatrixStack& getMatrixStack() const { return _matrixStack; }

protected:
    MatrixStack         _matrixStack;
    osg::BoundingSphere    _bb;
};

void OctreeAddRemove::recursivCellAddStaticObject(osg::BoundingSphere&bs, StaticOcclusionQueryNode &parent, osg::Group *child, osg::BoundingSphere& childbs)
{
    std::vector<StaticOcclusionQueryNode*> invalidbounds;
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

    if( bsi.radius() > mSettings.minOQNSize
        && target->getNumChildren() >= mSettings.maxOQNCapacity
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
            qnode->getQueryStateSet()->setRenderBinDetails( mSettings.OQRenderBin, "SORT_FRONT_TO_BACK", osg::StateSet::PROTECTED_RENDERBIN_DETAILS);

            //disable high BVH queries level
            unsigned int powlev = 1<<mSettings.maxBVHOQLevelCount;
            if( floor(bsi.radius() / mSettings.minOQNSize) >= powlev)
            {
                //OSG_WARN<<"masking high level OQN"<<floor(bsi.radius()/mSettings.minOQNSize)<<std::endl;
                qnode->getQueryGeometry()->setNodeMask(0);
                qnode->getDebugGeometry()->setNodeMask(0);
                parent.getQueryGeometry()->setNodeMask(0);
                parent.getDebugGeometry()->setNodeMask(0);

                parent.setQueriesEnabled(false);
                qnode->setQueriesEnabled(false);
            }
            else
            {
                qnode->getQueryGeometry()->setNodeMask(mSettings.OQMask);
                qnode->getDebugGeometry()->setNodeMask(mSettings.OQMask);
                qnode->setQueriesEnabled(true);
            }

            for(unsigned int i=0; i<target->getNumChildren(); ++i)
            {
                osg::Group * childi = target->getChild(i)->asGroup();
                OQGetBoundsVisitor boundvis2;
                childi->accept(boundvis2);
                osg::BoundingSphere bschild (boundvis2.getBoundingSphere());
                recursivCellAddStaticObject(bsi, *qnode, childi, bschild);
            }
            parent.setChild(ind, qnode);
        }

        recursivCellAddStaticObject(bsi, *qnode, child, childbs);
        invalidbounds.push_back(qnode);
    }
    else
    {
        target->addChild(child);
        invalidbounds.push_back(&parent);
    }
    for(auto qnode:invalidbounds)
    {
        qnode->resetStaticQueryGeometry();
    }
}


SettingsUpdatorVisitor::SettingsUpdatorVisitor(const OcclusionQuerySettings & settings): osg::NodeVisitor(osg::NodeVisitor::TRAVERSE_ALL_CHILDREN), mSettings(settings) {}

void SettingsUpdatorVisitor::apply(osg::OcclusionQueryNode&oqn)
{
    StaticOcclusionQueryNode *qnode = dynamic_cast<StaticOcclusionQueryNode*>(&oqn);
    if(qnode)
    {
        qnode->setQueryMargin(mSettings.querymargin);
        qnode->setVisibilityThreshold(mSettings.querypixelcount);
        qnode->setDebugDisplay(mSettings.debugDisplay);
        qnode->setQueryFrameCount(mSettings.queryframecount);
        qnode->setDistancePreventingPopin(mSettings.securepopdistance);
        qnode->getQueryStateSet()->setRenderBinDetails( mSettings.OQRenderBin, "SORT_FRONT_TO_BACK", osg::StateSet::PROTECTED_RENDERBIN_DETAILS);
    }
    traverse(oqn);
}

bool OctreeAddRemove::recursivCellRemoveStaticObject(StaticOcclusionQueryNode & parent, osg::Node * childtoremove)
{
    osg::Group * pchild; bool removed=false;
    for(unsigned int i=0; i< parent.getNumChildren(); ++i)
    {
        pchild = parent.getChild(i)->asGroup();
        if((removed = pchild->removeChild(childtoremove))) break;
    }

    if(removed)
    {
        StaticOcclusionQueryNode* curpar = &parent;

        while(curpar && curpar->getParent(0))
        {
            unsigned int capacity = 0;
            for(unsigned int i=0; i<8; ++i)
                capacity += curpar->getChild(i)->asGroup()->getNumChildren();
            /// TODO check other criterion for parent collapse
            if(capacity==0)
            {
                ///collapse parent
                OSG_NOTICE<<"collapse empty OQN"<<std::endl;
                osg::Group *paparent = curpar->getParent(0);
                paparent->setChild(paparent->getChildIndex(curpar), new osg::Group);
                curpar=dynamic_cast<StaticOcclusionQueryNode*>(paparent);
            }
            else break;
        }
        return true;
    }
    else
    {
        for(unsigned int i=0; i< parent.getNumChildren(); ++i)
        {
            StaticOcclusionQueryNode * child = dynamic_cast<StaticOcclusionQueryNode*>(parent.getChild(i));
            if(child && recursivCellRemoveStaticObject(*child, childtoremove))
                return true;
        }
    }
    return false;
}

}
