#include "material.hpp"

#include <osg/State>
#include <osg/StateSet>
#include <osg/Uniform>
namespace SceneUtil
{
    Material::Material(const MaterialConfig& config)
        : osg::StateAttribute()
        , mDiffuse(config.mDiffuse)
        , mAmbient(config.mAmbient)
        , mSpecular(config.mSpecular)
        , mEmission(config.mEmission)
        , mShininess(config.mShininess)
        , mEmissiveMult(config.mEmissiveMult)
        , mSpecularStrength(config.mSpecularStrength)
        , mVertexColorMode(config.mVertexColorMode)
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
        , mVertexColorMode(other.mVertexColorMode)
    {
    }

    void Material::apply(osg::State& state) const {}

    void Material::updateStateSet(osg::StateSet* stateset) const
    {
        stateset->getOrCreateUniform("material.diffuse", osg::Uniform::FLOAT_VEC4)->set(mDiffuse);
        stateset->getOrCreateUniform("material.ambient", osg::Uniform::FLOAT_VEC4)->set(mAmbient);
        stateset->getOrCreateUniform("material.specular", osg::Uniform::FLOAT_VEC4)->set(mSpecular);
        stateset->getOrCreateUniform("material.emission", osg::Uniform::FLOAT_VEC4)->set(mEmission);
        stateset->getOrCreateUniform("material.shininess", osg::Uniform::FLOAT)->set(mShininess);
        stateset->getOrCreateUniform("material.emissiveMult", osg::Uniform::FLOAT)->set(mEmissiveMult);
        stateset->getOrCreateUniform("material.specStrength", osg::Uniform::FLOAT)->set(mSpecularStrength);
        stateset->getOrCreateUniform("material.vertexColorMode", osg::Uniform::INT)
            ->set(static_cast<int>(mVertexColorMode));
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
        COMPARE_StateAttribute_Parameter(mVertexColorMode);

        return 0;
    }

    bool Material::operator==(const Material& other) const
    {
        return (mDiffuse == other.mDiffuse && mAmbient == other.mAmbient && mSpecular == other.mSpecular
            && mEmission == other.mEmission && mShininess == other.mShininess && mEmissiveMult == other.mEmissiveMult
            && mSpecularStrength == other.mSpecularStrength && mVertexColorMode == other.mVertexColorMode);
    }

}