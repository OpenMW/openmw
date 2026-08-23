
#ifndef OPENMW_COMPONENTS_SCENEUTIL_MATERIAL_H
#define OPENMW_COMPONENTS_SCENEUTIL_MATERIAL_H

#include <algorithm>
#include <cstdint>

#include <osg/StateAttribute>

namespace osg
{
    class Vec4f;
    class Uniform;
}

namespace SceneUtil
{
    enum class VertexColorModes : std::int32_t
    {
        None = 0,
        Emission = 1,
        AmbientAndDiffuse = 2,
        Ambient = 3,
        Diffuse = 4,
        Specular = 5,
    };

    struct MaterialConfig
    {
        osg::Vec4f mDiffuse = { 1, 1, 1, 1 };
        osg::Vec4f mAmbient = { 1, 1, 1, 1 };
        osg::Vec4f mSpecular = { 0, 0, 0, 0 };
        osg::Vec4f mEmission = { 0, 0, 0, 1 };
        float mShininess = 0.0;
        float mEmissiveMult = 1.0;
        float mSpecularStrength = 1.0;
        VertexColorModes mVertexColorMode = VertexColorModes::None;
    };

    class Material : public osg::StateAttribute
    {
    public:
        Material(const MaterialConfig& config = MaterialConfig{});

        Material(const Material& other, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        META_StateAttribute(SceneUtil, Material, osg::StateAttribute::MATERIAL)

        void apply(osg::State& state) const override;

        void updateStateSet(osg::StateSet* stateset) const;

        void setStateSet(
            osg::StateSet* stateset, osg::StateAttribute::OverrideValue overrideValue = osg::StateAttribute::ON) const;

        int compare(const StateAttribute& sa) const override;

        bool operator==(const Material& other) const;

        void setDiffuse(const osg::Vec4f& diffuse) { mConfig.mDiffuse = diffuse; }

        osg::Vec4f getDiffuse() const { return mConfig.mDiffuse; }

        void setAmbient(const osg::Vec4f& ambient) { mConfig.mAmbient = ambient; }

        osg::Vec4f getAmbient() const { return mConfig.mAmbient; }

        void setSpecular(const osg::Vec4f& specular) { mConfig.mSpecular = specular; }

        osg::Vec4f getSpecular() const { return mConfig.mSpecular; }

        void setEmission(const osg::Vec4f& emission) { mConfig.mEmission = emission; }

        osg::Vec4f getEmission() const { return mConfig.mEmission; }

        void setShininess(float shininess) { mConfig.mShininess = shininess; }

        float getShininess() const { return mConfig.mShininess; }

        void setEmissiveMultiplier(float mult) { mConfig.mEmissiveMult = mult; }

        float getEmissiveMultiplier() const { return mConfig.mEmissiveMult; }

        void setSpecularStrength(float strength) { mConfig.mSpecularStrength = strength; }

        float getSpecularStrength() const { return mConfig.mSpecularStrength; }

        void setAlpha(float alpha)
        {
            float clamped = std::clamp(alpha, 0.f, 1.f);
            mConfig.mDiffuse[3] = clamped;
            mConfig.mAmbient[3] = clamped;
            mConfig.mSpecular[3] = clamped;
            mConfig.mEmission[3] = clamped;
        }

        void setVertexColorMode(VertexColorModes mode) { mConfig.mVertexColorMode = mode; }

        VertexColorModes getVertexColorMode() const { return mConfig.mVertexColorMode; }

    private:
        MaterialConfig mConfig;
    };

}

#endif
