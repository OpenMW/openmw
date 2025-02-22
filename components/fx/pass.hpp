#ifndef OPENMW_COMPONENTS_FX_PASS_HPP
#define OPENMW_COMPONENTS_FX_PASS_HPP

#include <array>
#include <optional>
#include <string>
#include <string_view>
#include <unordered_set>

#include <osg/BlendEquation>
#include <osg/BlendFunc>
#include <osg/Shader>
#include <osg/Vec4f>
#include <osg/ref_ptr>

namespace osg
{
    class StateSet;
}

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

        Pass(Type type = Type::Pixel, Order order = Order::Post, bool ubo = false);

        void compile(Technique& technique, std::string_view preamble);

        std::string getTarget() const { return mTarget; }

        const std::array<std::string, 3>& getRenderTargets() const { return mRenderTargets; }

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

        std::array<std::string, 3> mRenderTargets;

        std::string mTarget;

        std::optional<osg::BlendFunc::BlendFuncMode> mBlendSource;
        std::optional<osg::BlendFunc::BlendFuncMode> mBlendDest;
        std::optional<osg::BlendEquation::Equation> mBlendEq;
    };
}

#endif
