#ifndef OCCLUSION_QUERY_OBJECTS_H
#define OCCLUSION_QUERY_OBJECTS_H

#include <osg/Version>
#include <osg/Group>
#include <osg/Geometry>
#include <osg/Geode>

#include <osg/OcclusionQueryNode>

namespace SceneUtil{

class TestResult : public osg::Referenced
{
public:
    TestResult() : _init( false ), _id( 0 ), _contextID( 0 ), _active( false ), _lastresultavailable( false ), _numPixels( 0 ) {setThreadSafeRefUnref(true);}
    ~TestResult() {}

    bool _init;

    // Query ID for this context.
    GLuint _id;
    // Context ID owning this query ID.
    unsigned int _contextID;

    // Set to true when a query gets issued and set to
    //   false when the result is retrieved.
    mutable bool _active;
    mutable bool _lastresultavailable;
    // Result of last query.
    GLint _numPixels;
};

// QueryGeometry -- A Drawable that performs an occlusion query,
//   using its geometric data as the query geometry.
class StaticOcclusionQueryNode;
class  MWQueryGeometry : public osg::QueryGeometry
{
public:
    MWQueryGeometry(const std::string& oqnName=std::string(""));
    ~MWQueryGeometry();
    virtual void drawImplementation( osg::RenderInfo& renderInfo ) const;

    struct QueryResult
    {
        QueryResult() : valid(false), numPixels(0) {}
        QueryResult(bool v, unsigned int p) : valid(v), numPixels(p) {}

        bool valid;
        unsigned int numPixels;
    };

    /** return a QueryResult for specified Camera, where the QueryResult.valid is true when query results are available, and in which case the QueryResult.numPixels provides the num of pixels in the query result.*/
    QueryResult getMWQueryResult( const osg::Camera* cam );

    unsigned int getNumPixels( const osg::Camera* cam );

    virtual void releaseGLObjects( osg::State* state = 0 ) const;

    static void deleteQueryObject( unsigned int contextID, GLuint handle );
    static void flushDeletedQueryObjects( unsigned int contextID, double currentTime, double& availableTime );
    static void discardDeletedQueryObjects( unsigned int contextID );

protected:
    typedef std::map< const osg::Camera*, osg::ref_ptr<TestResult> > ResultMap;
    mutable ResultMap _mwresults;
};
class StaticOcclusionQueryNode : public osg::OcclusionQueryNode
{
public:

    StaticOcclusionQueryNode():osg::OcclusionQueryNode(), _margin(0.0f), _isgetpassedearlyexitenable(true)
#if  OSG_VERSION_LESS_THAN(3,6,4)
      , _validQueryGeometry(false)
#endif
    {
        createSupportNodes();
        getQueryGeometry()->setUseVertexBufferObjects(true);
        setDataVariance(osg::Object::STATIC);
    }

    //set MainCamera (from which passed is updated)
    void setMainViewCamera(osg::Camera*cam){ _maincam = cam;}
    const osg::Camera* getMainViewCamera() const { return _maincam; }

    inline osg::Geometry* getDebugGeometry() { return static_cast<osg::Geometry*>(_debugGeode->getChild(0)); }
    inline void setQueryMargin(float m) { _margin = m; }
    inline float getQueryMargin() const { return _margin; }

    //enable or disable early exit for not continuous OQN (under a lod or switch)
    //NB: disable it for hierarchical OQN
    inline void setEarlyExitOn(bool m) { _isgetpassedearlyexitenable = m; }
    inline float getEarlyExit() const { return _isgetpassedearlyexitenable; }

    inline void invalidateQueryGeometry() {  _validQueryGeometry = false;  }


    virtual void createSupportNodes();
    virtual osg::BoundingSphere computeBound() const;
    virtual bool getPassed( const osg::Camera* camera, osg::NodeVisitor& nv );

protected:
    static osg::ref_ptr< osg::StateSet > OQStateSet;
    static osg::ref_ptr< osg::StateSet > OQDebugStateSet;
    osg::StateSet *initMWOQState();
    osg::StateSet *initMWOQDebugState();
    float _margin;
    bool _isgetpassedearlyexitenable;
    osg::ref_ptr< osg::Camera > _maincam;
#if  OSG_VERSION_LESS_THAN(3,6,4)
    mutable bool _validQueryGeometry;
#endif

};

struct OcclusionQuerySettings
{
    bool enable;
    bool debugDisplay;
    float maxCellSize;
    unsigned int querypixelcount;
    unsigned int queryframecount;
    float querymargin;
    unsigned int maxBVHOQLevelCount;
    ///subdivision criterions
    float minOQNSize;
    unsigned int maxDrawablePerOQN;

};
struct OctreeAddRemove
{
    OctreeAddRemove(const OcclusionQuerySettings & settings): mSettings(settings) {}
    const OcclusionQuerySettings & mSettings;

    void recursivCellAddStaticObject(osg::BoundingSphere&bs, StaticOcclusionQueryNode &parent, osg::Group *child, osg::BoundingSphere& childbs);

    bool recursivCellRemoveStaticObject(osg::OcclusionQueryNode & parent, osg::Node * childtoremove);
};
}
#endif
