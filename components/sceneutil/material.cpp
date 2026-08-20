#include "material.hpp"

#include <osg/State>
#include <osg/StateSet>
#include <osg/Uniform>
namespace SceneUtil
{
    Material::Material(const MaterialConfig& config)
        : osg::StateAttribute()
        , mConfig(config)
    {
    }

    Material::Material(const Material& other, const osg::CopyOp& copyop)
        : osg::StateAttribute(other, copyop)
        , mConfig(other.mConfig)
    {
    }

    void Material::apply(osg::State& state) const {}

    void Material::setStateSet(osg::StateSet* stateset, osg::StateAttribute::OverrideValue value) const
    {
        stateset->addUniform(new osg::Uniform("material.diffuse", mConfig.mDiffuse), value);
        stateset->addUniform(new osg::Uniform("material.ambient", mConfig.mAmbient), value);
        stateset->addUniform(new osg::Uniform("material.specular", mConfig.mSpecular), value);
        stateset->addUniform(new osg::Uniform("material.emission", mConfig.mEmission), value);
        stateset->addUniform(new osg::Uniform("material.shininess", mConfig.mShininess), value);
        stateset->addUniform(new osg::Uniform("material.emissiveMult", mConfig.mEmissiveMult), value);
        stateset->addUniform(new osg::Uniform("material.specStrength", mConfig.mSpecularStrength), value);
        stateset->addUniform(
            new osg::Uniform("material.vertexColorMode", static_cast<int>(mConfig.mVertexColorMode)), value);
    }

    void Material::updateStateSet(osg::StateSet* stateset) const
    {
        stateset->getOrCreateUniform("material.diffuse", osg::Uniform::FLOAT_VEC4)->set(mConfig.mDiffuse);
        stateset->getOrCreateUniform("material.ambient", osg::Uniform::FLOAT_VEC4)->set(mConfig.mAmbient);
        stateset->getOrCreateUniform("material.specular", osg::Uniform::FLOAT_VEC4)->set(mConfig.mSpecular);
        stateset->getOrCreateUniform("material.emission", osg::Uniform::FLOAT_VEC4)->set(mConfig.mEmission);
        stateset->getOrCreateUniform("material.shininess", osg::Uniform::FLOAT)->set(mConfig.mShininess);
        stateset->getOrCreateUniform("material.emissiveMult", osg::Uniform::FLOAT)->set(mConfig.mEmissiveMult);
        stateset->getOrCreateUniform("material.specStrength", osg::Uniform::FLOAT)->set(mConfig.mSpecularStrength);
        stateset->getOrCreateUniform("material.vertexColorMode", osg::Uniform::INT)
            ->set(static_cast<int>(mConfig.mVertexColorMode));
    }

    int Material::compare(const StateAttribute& sa) const
    {
        COMPARE_StateAttribute_Types(Material, sa);
        COMPARE_StateAttribute_Parameter(mConfig.mDiffuse);
        COMPARE_StateAttribute_Parameter(mConfig.mAmbient);
        COMPARE_StateAttribute_Parameter(mConfig.mSpecular);
        COMPARE_StateAttribute_Parameter(mConfig.mEmission);
        COMPARE_StateAttribute_Parameter(mConfig.mShininess);
        COMPARE_StateAttribute_Parameter(mConfig.mEmissiveMult);
        COMPARE_StateAttribute_Parameter(mConfig.mSpecularStrength);
        COMPARE_StateAttribute_Parameter(mConfig.mVertexColorMode);

        return 0;
    }

    bool Material::operator==(const Material& other) const
    {
        return (mConfig.mDiffuse == other.mConfig.mDiffuse && mConfig.mAmbient == other.mConfig.mAmbient
            && mConfig.mSpecular == other.mConfig.mSpecular && mConfig.mEmission == other.mConfig.mEmission
            && mConfig.mShininess == other.mConfig.mShininess && mConfig.mEmissiveMult == other.mConfig.mEmissiveMult
            && mConfig.mSpecularStrength == other.mConfig.mSpecularStrength
            && mConfig.mVertexColorMode == other.mConfig.mVertexColorMode);
    }

}