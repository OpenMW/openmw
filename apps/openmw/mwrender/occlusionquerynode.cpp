#include "occlusionquerynode.hpp"

#include <OpenThreads/ScopedLock>
#include <osg/Timer>
#include <osg/Notify>
#include <osg/ComputeBoundsVisitor>
#include <osg/PolygonMode>
#include <osg/ColorMask>
#include <osg/PolygonOffset>
#include <osg/Depth>

#include "renderbin.hpp"

using namespace osg;

namespace MWRender
{

GLushort cubeindices[] = { 0, 1, 2, 3,  4, 5, 6, 7,
    0, 3, 6, 5,  2, 1, 4, 7,
    5, 4, 1, 0,  2, 7, 6, 3 };

osg::ref_ptr< osg::StateSet > StaticOcclusionQueryNode::OQDebugStateSet = 0;
osg::ref_ptr< osg::StateSet > StaticOcclusionQueryNode::OQStateSet = 0;

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

            osg::ref_ptr<osg::Vec3Array> v = new osg::Vec3Array( 8 );
            Vec3Array::iterator itv = v->begin();
            // Having (0,0,0) as vertices for the case of the invalid query geometry
            // still isn't quite the right thing. But the query geometry is public
            // accessible and therefore a user might expect eight vertices, so
            // it seems safer to keep eight vertices in the geometry.

            osg::Geometry* geom = static_cast<osg:: Geometry* >( nonConstThis->_queryGeode->getDrawable( 0 ) );
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
                geom->setVertexArray( v.get() );

                geom = static_cast< osg::Geometry* >( nonConstThis->_debugGeode->getDrawable( 0 ) );
                geom->setVertexArray( v.get() );
            }

    }

    return Group::computeBound();
}


void StaticOcclusionQueryNode::createSupportNodes()
{
    osg::DrawElementsUShort * dr = new osg::DrawElementsUShort( osg::PrimitiveSet::QUADS, 24,  cubeindices );
    dr->setElementBufferObject(new osg::ElementBufferObject());
    {
        // Add the test geometry Geode
        _queryGeode = new osg::Geode;
        _queryGeode->setName( "OQTest" );
        _queryGeode->setDataVariance( Object::STATIC );

        osg::ref_ptr<osg::QueryGeometry > geom = new osg::QueryGeometry( /*this,*/getName() );
       geom->setDataVariance( Object::STATIC );
        geom->addPrimitiveSet(  dr);
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

        osg::ref_ptr<osg::Vec4Array> ca = new osg::Vec4Array;
        ca->push_back( osg::Vec4( 1.f, 1.f, 1.f, 1.f ) );
        geom->setColorArray( ca.get(), osg::Array::BIND_OVERALL );

        geom->addPrimitiveSet(dr );

        _debugGeode->addDrawable( geom.get() );
    }

    // Creste state sets. Note that the osgOQ visitors (which place OQNs throughout
    //   the scene graph) create a single instance of these StateSets shared
    //   between all OQNs for efficiency.
    setQueryStateSet( initMWOQState() );
    setDebugStateSet( initMWOQDebugState() );
}

}
