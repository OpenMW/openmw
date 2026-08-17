#include <format>

#include <osg/Matrixf>
#include <osg/StateSet>
#include <osg/Uniform>

namespace SceneUtil
{

    osg::ref_ptr<osg::Uniform> createTexMatUniform(unsigned int texUnit, const osg::Matrixf& mat)
    {
        return new osg::Uniform(std::format("texMat{}", texUnit).c_str(), mat);
    }

    void setupTexMatForStateSet(osg::StateSet& stateset, unsigned int texUnit, const osg::Matrixf& mat)
    {
        stateset.getOrCreateUniform(std::format("texMat{}", texUnit), osg::Uniform::FLOAT_MAT4)->set(mat);
    }

    void initTexMatForStateSet(osg::StateSet& stateset)
    {
        constexpr unsigned int maxTexMat = 10;
        for (unsigned int i = 0; i < maxTexMat; ++i)
            setupTexMatForStateSet(stateset, i, osg::Matrixf{});
    }

}
