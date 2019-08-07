#ifndef OPENMW_COMPONENTS_SCENEUTIL_UTIL_H
#define OPENMW_COMPONENTS_SCENEUTIL_UTIL_H

#include <osg/Matrix>
#include <osg/BoundingSphere>
#include <osg/NodeCallback>
#include <osg/Texture2D>
#include <osg/Vec4f>

#include <components/resource/resourcesystem.hpp>

#include "statesetupdater.hpp"

namespace SceneUtil
{
    class GlowUpdater : public SceneUtil::StateSetUpdater
    {
    public:
        GlowUpdater(int texUnit, const osg::Vec4f& color, const std::vector<osg::ref_ptr<osg::Texture2D> >& textures,
            osg::Node* node, float duration, Resource::ResourceSystem* resourcesystem);

        virtual void setDefaults(osg::StateSet *stateset);

        void removeTexture(osg::StateSet* stateset);
        virtual void apply(osg::StateSet *stateset, osg::NodeVisitor *nv);

        bool isPermanentGlowUpdater();

        bool isDone();

        void setColor(const osg::Vec4f& color);

        void setDuration(float duration);

    private:
        int mTexUnit;
        osg::Vec4f mColor;
        osg::Vec4f mOriginalColor; // for restoring the color of a permanent glow after a temporary glow on the object finishes
        std::vector<osg::ref_ptr<osg::Texture2D> > mTextures;
        osg::Node* mNode;
        float mDuration;
        float mOriginalDuration; // for recording that this is originally a permanent glow if it is changed to a temporary one
        float mStartingTime;
        Resource::ResourceSystem* mResourceSystem;
        bool mColorChanged;
        bool mDone;
    };

    // Transform a bounding sphere by a matrix
    // based off private code in osg::Transform
    // TODO: patch osg to make public
    void transformBoundingSphere (const osg::Matrixf& matrix, osg::BoundingSphere& bsphere);

    osg::Vec4f colourFromRGB (unsigned int clr);

    osg::Vec4f colourFromRGBA (unsigned int value);

    float makeOsgColorComponent (unsigned int value, unsigned int shift);

    bool hasUserDescription(const osg::Node* node, const std::string pattern);

    osg::ref_ptr<GlowUpdater> addEnchantedGlow(osg::ref_ptr<osg::Node> node, Resource::ResourceSystem* resourceSystem, osg::Vec4f glowColor, float glowDuration=-1);
}

#endif
