#ifndef OCCLUSION_QUERY_OBJECTS_H
#define OCCLUSION_QUERY_OBJECTS_H

#include <osg/Version>
#include <osg/Group>
#include <osg/Geometry>
#include <osg/Geode>

#include <osg/OcclusionQueryNode>

namespace SceneUtil{

class StaticOcclusionQueryNode : public osg::OcclusionQueryNode
{
public:

    StaticOcclusionQueryNode():osg::OcclusionQueryNode(), _margin(0.0f)
#if  OSG_VERSION_LESS_THAN(3,6,4)
      , _validQueryGeometry(false)
#endif
    {
        createSupportNodes();
        getQueryGeometry()->setUseVertexBufferObjects(true);
        setDataVariance(osg::Object::STATIC);
    }
    inline osg::Geometry* getDebugGeometry() { return static_cast<osg::Geometry*>(_debugGeode->getChild(0)); }
    inline void setQueryMargin(float m) { _margin = m; }
    inline float getQueryMargin() const { return _margin; }
    inline void invalidateQueryGeometry() {  _validQueryGeometry = false;  }

    virtual void createSupportNodes();
    virtual osg::BoundingSphere computeBound() const;

protected:
    static osg::ref_ptr< osg::StateSet > OQStateSet;
    static osg::ref_ptr< osg::StateSet > OQDebugStateSet;
    osg::StateSet *initMWOQState();
    osg::StateSet *initMWOQDebugState();
    float _margin;
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
