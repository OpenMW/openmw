
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

    class Material : public osg::StateAttribute
    {
    public:
        Material();

        Material(const Material& other, const osg::CopyOp& copyop = osg::CopyOp::SHALLOW_COPY);

        META_StateAttribute(SceneUtil, Material, osg::StateAttribute::MATERIAL)

        void apply(osg::State& state) const override;

        void updateStateSet(osg::StateSet* stateset) const;

        int compare(const StateAttribute& sa) const override;

        bool operator==(const Material& other) const;

        void setDiffuse(const osg::Vec4f& diffuse) { mDiffuse = diffuse; }

        osg::Vec4f getDiffuse() const { return mDiffuse; }

        void setAmbient(const osg::Vec4f& ambient) { mAmbient = ambient; }

        osg::Vec4f getAmbient() const { return mAmbient; }

        void setSpecular(const osg::Vec4f& specular) { mSpecular = specular; }

        osg::Vec4f getSpecular() const { return mSpecular; }

        void setEmission(const osg::Vec4f& emission) { mEmission = emission; }

        osg::Vec4f getEmission() const { return mEmission; }

        void setShininess(float shininess) { mShininess = shininess; }

        float getShininess() const { return mShininess; }

        void setEmissiveMultiplier(float mult) { mEmissiveMult = mult; }

        float getEmissiveMultiplier() const { return mEmissiveMult; }

        void setSpecularStrength(float strength) { mSpecularStrength = strength; }

        float getSpecularStrength() const { return mSpecularStrength; }

        void setAlpha(float alpha)
        {
            float clamped = std::clamp(alpha, 0.f, 1.f);
            mDiffuse[3] = clamped;
            mAmbient[3] = clamped;
            mSpecular[3] = clamped;
            mEmission[3] = clamped;
        }

        void setVertexColorMode(VertexColorModes mode) { mVertexColorMode = mode; }

        VertexColorModes getVertexColorMode() const { return mVertexColorMode; }

    private:
        osg::Vec4f mDiffuse;
        osg::Vec4f mAmbient;
        osg::Vec4f mSpecular;
        osg::Vec4f mEmission;
        float mShininess = 0.0;
        float mEmissiveMult = 1.0;
        float mSpecularStrength = 1.0;
        VertexColorModes mVertexColorMode = VertexColorModes::None;
    };

}

#endif