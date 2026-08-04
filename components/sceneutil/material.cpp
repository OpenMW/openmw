#include "material.hpp"

#include <osg/State>
#include <osg/StateSet>
#include <osg/Uniform>
namespace SceneUtil
{
    Material::Material()
        : osg::StateAttribute()
    {
    }

    Material::Material(const Material& other, const osg::CopyOp& copyop)
        : osg::StateAttribute(other, copyop)
        , mDiffuse(other.mDiffuse)
        , mAmbient(other.mAmbient)
        , mSpecular(other.mSpecular)
        , mEmission(other.mEmission)
        , mShininess(other.mShininess)
        , mEmissiveMult(other.mEmissiveMult)
        , mSpecularStrength(other.mSpecularStrength)
        , mColorMode(other.mColorMode)
    {
    }

    void Material::apply(osg::State& state) const {}

    void Material::apply(osg::StateSet* stateset) const
    {
        stateset->getOrCreateUniform("material.diffuse", osg::Uniform::FLOAT_VEC4)->set(mDiffuse);
        stateset->getOrCreateUniform("material.ambient", osg::Uniform::FLOAT_VEC4)->set(mAmbient);
        stateset->getOrCreateUniform("material.specular", osg::Uniform::FLOAT_VEC4)->set(mSpecular);
        stateset->getOrCreateUniform("material.emission", osg::Uniform::FLOAT_VEC4)->set(mEmission);
        stateset->getOrCreateUniform("material.shininess", osg::Uniform::FLOAT)->set(mShininess);
        stateset->getOrCreateUniform("material.emissiveMult", osg::Uniform::FLOAT)->set(mEmissiveMult);
        stateset->getOrCreateUniform("material.specStrength", osg::Uniform::FLOAT)->set(mSpecularStrength);
        stateset->getOrCreateUniform("material.colorMode", osg::Uniform::INT)->set(static_cast<int>(mColorMode));
    }

    int Material::compare(const StateAttribute& sa) const
    {
        COMPARE_StateAttribute_Types(Material, sa);
        COMPARE_StateAttribute_Parameter(mDiffuse);
        COMPARE_StateAttribute_Parameter(mAmbient);
        COMPARE_StateAttribute_Parameter(mSpecular);
        COMPARE_StateAttribute_Parameter(mEmission);
        COMPARE_StateAttribute_Parameter(mShininess);
        COMPARE_StateAttribute_Parameter(mEmissiveMult);
        COMPARE_StateAttribute_Parameter(mSpecularStrength);
        COMPARE_StateAttribute_Parameter(mColorMode);

        return 0;
    }

    bool Material::operator==(const Material& other) const
    {
        return (mDiffuse == other.mDiffuse && mAmbient == other.mAmbient && mSpecular == other.mSpecular
            && mEmission == other.mEmission && mShininess == other.mShininess && mEmissiveMult == other.mEmissiveMult
            && mSpecularStrength == other.mSpecularStrength && mColorMode == other.mColorMode);
    }

}