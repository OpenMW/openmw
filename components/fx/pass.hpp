#ifndef OPENMW_COMPONENTS_FX_PASS_H
#define OPENMW_COMPONENTS_FX_PASS_H

#include <array>
#include <string>
#include <sstream>
#include <cstdint>
#include <unordered_set>
#include <optional>

#include <osg/Timer>
#include <osg/Program>
#include <osg/Shader>
#include <osg/State>
#include <osg/Texture2D>
#include <osg/BlendEquation>
#include <osg/BlendFunc>

namespace fx
{
    class Technique;

    class Pass
    {
    public:

        enum class Order
        {
            Forward,
            Post
        };

        enum class Type
        {
            None,
            Pixel,
            Compute
        };

        friend class Technique;

        Pass(Type type=Type::Pixel, Order order=Order::Post, bool ubo = false);

        void compile(Technique& technique, std::string_view preamble);

        std::string_view getTarget() const { return mTarget; }

        void prepareStateSet(osg::StateSet* stateSet, const std::string& name) const;

        std::string getName() const { return mName; }

        void dirty();

    private:
        std::string getPassHeader(Technique& technique, std::string_view preamble, bool fragOut = false);

        bool mCompiled;

        osg::ref_ptr<osg::Shader> mVertex;
        osg::ref_ptr<osg::Shader> mFragment;
        osg::ref_ptr<osg::Shader> mCompute;

        Type mType;
        Order mOrder;
        std::string mName;
        bool mLegacyGLSL;
        bool mUBO;
        bool mSupportsNormals;

        std::string_view mTarget;
        std::optional<osg::Vec4f> mClearColor;

        std::optional<osg::BlendFunc::BlendFuncMode> mBlendSource;
        std::optional<osg::BlendFunc::BlendFuncMode> mBlendDest;
        std::optional<osg::BlendEquation::Equation> mBlendEq;
    };
}

#endif
