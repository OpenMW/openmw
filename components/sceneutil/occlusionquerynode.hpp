#ifndef OCCLUSION_QUERY_OBJECTS_H
#define OCCLUSION_QUERY_OBJECTS_H

#include <osg/Version>
#include <osg/Group>
#include <osg/Geometry>
#include <osg/Geode>

#include <osg/OcclusionQueryNode>
#include <osg/ComputeBoundsVisitor>

#include "occlusionsettings.hpp"

namespace SceneUtil
{

class TestResult : public osg::Referenced
{
public:
    TestResult() : _init( false ), _id( 0 ), _contextID( 0 ), _active( false ), _numPixels( 0 ), _lastnumPixels(0) {setThreadSafeRefUnref(true);}
    ~TestResult() {}

    bool _init;

    // Query ID for this context.
    GLuint _id;
    // Context ID owning this query ID.
    unsigned int _contextID;

    // Set to true when a query gets issued and set to
    //   false when the result is retrieved.
    mutable bool _active;
    // Result of last query.
    GLint _numPixels, _lastnumPixels;
};

// QueryGeometry -- A Drawable that performs an occlusion query,
//   using its geometric data as the query geometry.
class StaticOcclusionQueryNode;
class MWQueryGeometry : public osg::QueryGeometry
{
public:
    MWQueryGeometry(const std::string& oqnName=std::string(""));
    ~MWQueryGeometry();
    virtual void drawImplementation( osg::RenderInfo& renderInfo ) const;

    struct QueryResult
    {
        QueryResult() : valid(false), numPixels(0), lastnumPixels(0) {}
        QueryResult(bool v, unsigned int p, unsigned int l) : valid(v), numPixels(p), lastnumPixels(l) {}

        bool valid;
        unsigned int numPixels, lastnumPixels;
    };

    /** return a QueryResult for specified Camera, where the QueryResult.valid is true when query results are available, and in which case the QueryResult.numPixels provides the num of pixels in the query result.*/
    QueryResult getMWQueryResult( const osg::Camera* cam ) const;

    unsigned int getLastQueryNumPixels( const osg::Camera* cam ) const;
    unsigned int getNumPixels( const osg::Camera* cam ) const;

    void forceQueryResult( const osg::Camera* cam, unsigned int numPixels) const;

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

    StaticOcclusionQueryNode():osg::OcclusionQueryNode(), _margin(0.0f), _securepopdistance(0.0f)
#if OSG_VERSION_LESS_THAN(3,6,5)
      , _queryGeometryState(INVALID)
#endif
    {
        createSupportNodes();
        _maincam = defaultMainCamera;
    }

    static osg::Camera * defaultMainCamera;
    //set MainCamera (from which passed is updated)
    void setMainViewCamera(osg::Camera * cam){ _maincam = cam;}
    const osg::Camera* getMainViewCamera() const { return _maincam; }

    inline void setQueryMargin(float m) { _margin = m; }
    inline float getQueryMargin() const { return _margin; }

    //enable or disable early exit for not continuous OQN (under a lod or switch)
    //NB: disable it for hierarchical OQN
    inline void setDistancePreventingPopin(float m) { _securepopdistance = m; }
    inline float getDistancePreventingPopin() const { return _securepopdistance; }

    virtual bool getPassed( const osg::Camera* camera, osg::NodeVisitor& nv );

    /// rebuild QueryGeometry based on children bounds
    void resetStaticQueryGeometry();

    inline osg::Geometry* getDebugGeometry() { return static_cast<osg::Geometry*>(_debugGeode->getChild(0)); }
    inline MWQueryGeometry* getQueryGeometry() { return static_cast<MWQueryGeometry*>(_queryGeode->getChild(0)); }

protected:

    virtual void createSupportNodes();

    void pullUpVisibility(const osg::Camera*cam, unsigned int numPix);
    void pullDownVisibility(const osg::Camera*cam, unsigned int numPix);

    static osg::ref_ptr< osg::StateSet > OQStateSet;
    static osg::ref_ptr< osg::StateSet > OQDebugStateSet;
    osg::StateSet *initMWOQState();
    osg::StateSet *initMWOQDebugState();

    float _margin;
    osg::Matrix::value_type _securepopdistance;
    osg::ref_ptr< osg::Camera > _maincam;
    std::map< const osg::Camera*, unsigned int > _lastframes;
#if  OSG_VERSION_LESS_THAN(3,6,5)
    enum QueryGeometryState {
        INVALID,
        VALID,
        USER_DEFINED
    };
    mutable QueryGeometryState _queryGeometryState;
    inline bool isQueryGeometryValid() const { return _queryGeometryState != INVALID; }
    virtual osg::BoundingSphere computeBound() const { return Group::computeBound(); }
#endif
};

/// get scene bounding box based on StaticOcclusionQueryNode::getMainViewCamera occlusions result
class OQComputeBoundsVisitor : public osg::ComputeBoundsVisitor
{
public:

    META_NodeVisitor(osgMW, OQComputeBoundsVisitor)
    OQComputeBoundsVisitor(TraversalMode traversalMode = TRAVERSE_ALL_CHILDREN): osg::ComputeBoundsVisitor(traversalMode) { }

    virtual void apply(osg::OcclusionQueryNode& node)
    {
        if(node.getPassed())
            traverse(node);
    }
};

struct SettingsUpdatorVisitor : public osg::NodeVisitor
{
    SettingsUpdatorVisitor(const OcclusionQuerySettings & settings);
    const OcclusionQuerySettings & mSettings;
    void apply(osg::OcclusionQueryNode&oqn);
};

struct OctreeAddRemove
{
    OctreeAddRemove(const OcclusionQuerySettings & settings): mSettings(settings) {}
    const OcclusionQuerySettings & mSettings;

    void recursivCellAddStaticObject(osg::BoundingSphere&bs, StaticOcclusionQueryNode &parent, osg::Group *child, osg::BoundingSphere& childbs);    
    bool recursivCellRemoveStaticObject(StaticOcclusionQueryNode &parent, osg::Node * childtoremove);
};
}
#endif
