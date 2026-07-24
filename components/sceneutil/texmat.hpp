#ifndef OPENMW_COMPONENTS_SCENEUTIL_TEXMAT_H
#define OPENMW_COMPONENTS_SCENEUTIL_TEXMAT_H

#include <osg/ref_ptr>

namespace osg
{
    class Matrixf;
    class Uniform;
    class StateSet;
}

namespace SceneUtil
{

    osg::ref_ptr<osg::Uniform> createTexMatUniform(unsigned int texUnit, const osg::Matrixf& mat);

    void setupTexMatForStateSet(osg::StateSet& stateset, unsigned int texUnit, const osg::Matrixf& mat);

}

#endif
