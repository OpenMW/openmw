#ifndef CULLVISITOREX_H
#define CULLVISITOREX_H

#include <osgUtil/CullVisitor>

/// Needed for mixing osg rendering with Qt 2D drawing using QPainter...
/// See http://forum.openscenegraph.org/viewtopic.php?t=15627&view=previous

class CullVisitorEx : public osgUtil::CullVisitor
{
public:
    META_NodeVisitor(Ex, CullVisitorEx)

    CullVisitorEx() {}
    CullVisitorEx(const CullVisitorEx& cv) : osgUtil::CullVisitor(cv) { }
    CullVisitorEx* clone() const
    {
        return new CullVisitorEx(*this);
    }

    virtual void apply(osg::Camera& camera);
};

#endif // CULLVISITOREX_H
