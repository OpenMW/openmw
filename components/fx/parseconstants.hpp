#ifndef OPENMW_COMPONENTS_FX_PARSECONSTANTS_HPP
#define OPENMW_COMPONENTS_FX_PARSECONSTANTS_HPP

#include <array>
#include <string_view>

#include <osg/BlendEquation>
#include <osg/BlendFunc>
#include <osg/Image>
#include <osg/Texture>

#include <components/sceneutil/color.hpp>

#include "technique.hpp"

namespace fx
{
    namespace constants
    {
        constexpr std::array<std::pair<std::string_view, fx::FlagsType>, 6> TechniqueFlag = { {
            { "disable_interiors", Technique::Flag_Disable_Interiors },
            { "disable_exteriors", Technique::Flag_Disable_Exteriors },
            { "disable_underwater", Technique::Flag_Disable_Underwater },
            { "disable_abovewater", Technique::Flag_Disable_Abovewater },
            { "disable_sunglare", Technique::Flag_Disable_SunGlare },
            { "hidden", Technique::Flag_Hidden },
        } };

        constexpr std::array<std::pair<std::string_view, int>, 6> SourceFormat = { {
            { "red", GL_RED },
            { "rg", GL_RG },
            { "rgb", GL_RGB },
            { "bgr", GL_BGR },
            { "rgba", GL_RGBA },
            { "bgra", GL_BGRA },
        } };

        constexpr std::array<std::pair<std::string_view, int>, 10> SourceType = { {
            { "byte", GL_BYTE },
            { "unsigned_byte", GL_UNSIGNED_BYTE },
            { "short", GL_SHORT },
            { "unsigned_short", GL_UNSIGNED_SHORT },
            { "int", GL_INT },
            { "unsigned_int", GL_UNSIGNED_INT },
            { "unsigned_int_24_8", GL_UNSIGNED_INT_24_8 },
            { "half_float", GL_HALF_FLOAT },
            { "float", GL_FLOAT },
            { "double", GL_DOUBLE },
        } };

        constexpr std::array<std::pair<std::string_view, int>, 16> InternalFormat = { {
            { "red", GL_RED },
            { "r16f", GL_R16F },
            { "r32f", GL_R32F },
            { "rg", GL_RG },
            { "rg16f", GL_RG16F },
            { "rg32f", GL_RG32F },
            { "rgb", GL_RGB },
            { "rgb16f", GL_RGB16F },
            { "rgb32f", GL_RGB32F },
            { "rgba", GL_RGBA },
            { "rgba16f", GL_RGBA16F },
            { "rgba32f", GL_RGBA32F },
            { "depth_component16", GL_DEPTH_COMPONENT16 },
            { "depth_component24", GL_DEPTH_COMPONENT24 },
            { "depth_component32", GL_DEPTH_COMPONENT32 },
            { "depth_component32f", GL_DEPTH_COMPONENT32F },
        } };

        constexpr std::array<std::pair<std::string_view, osg::Texture::InternalFormatMode>, 13> Compression = { {
            { "auto", osg::Texture::USE_USER_DEFINED_FORMAT },
            { "arb", osg::Texture::USE_ARB_COMPRESSION },
            { "s3tc_dxt1", osg::Texture::USE_S3TC_DXT1_COMPRESSION },
            { "s3tc_dxt3", osg::Texture::USE_S3TC_DXT3_COMPRESSION },
            { "s3tc_dxt5", osg::Texture::USE_S3TC_DXT5_COMPRESSION },
            { "pvrtc_2bpp", osg::Texture::USE_PVRTC_2BPP_COMPRESSION },
            { "pvrtc_4bpp", osg::Texture::USE_PVRTC_4BPP_COMPRESSION },
            { "etc", osg::Texture::USE_ETC_COMPRESSION },
            { "etc2", osg::Texture::USE_ETC2_COMPRESSION },
            { "rgtc1", osg::Texture::USE_RGTC1_COMPRESSION },
            { "rgtc2", osg::Texture::USE_RGTC2_COMPRESSION },
            { "s3tc_dxt1c", osg::Texture::USE_S3TC_DXT1c_COMPRESSION },
            { "s3tc_dxt1a", osg::Texture::USE_S3TC_DXT1a_COMPRESSION },
        } };

        constexpr std::array<std::pair<std::string_view, osg::Texture::WrapMode>, 4> WrapMode = { {
            { "clamp_to_edge", osg::Texture::CLAMP_TO_EDGE },
            { "clamp_to_border", osg::Texture::CLAMP_TO_BORDER },
            { "repeat", osg::Texture::REPEAT },
            { "mirror", osg::Texture::MIRROR },
        } };

        constexpr std::array<std::pair<std::string_view, osg::Texture::FilterMode>, 6> FilterMode = { {
            { "linear", osg::Texture::LINEAR },
            { "linear_mipmap_linear", osg::Texture::LINEAR_MIPMAP_LINEAR },
            { "linear_mipmap_nearest", osg::Texture::LINEAR_MIPMAP_NEAREST },
            { "nearest", osg::Texture::NEAREST },
            { "nearest_mipmap_linear", osg::Texture::NEAREST_MIPMAP_LINEAR },
            { "nearest_mipmap_nearest", osg::Texture::NEAREST_MIPMAP_NEAREST },
        } };

        constexpr std::array<std::pair<std::string_view, osg::BlendFunc::BlendFuncMode>, 15> BlendFunc = { {
            { "dst_alpha", osg::BlendFunc::DST_ALPHA },
            { "dst_color", osg::BlendFunc::DST_COLOR },
            { "one", osg::BlendFunc::ONE },
            { "one_minus_dst_alpha", osg::BlendFunc::ONE_MINUS_DST_ALPHA },
            { "one_minus_dst_color", osg::BlendFunc::ONE_MINUS_DST_COLOR },
            { "one_minus_src_alpha", osg::BlendFunc::ONE_MINUS_SRC_ALPHA },
            { "one_minus_src_color", osg::BlendFunc::ONE_MINUS_SRC_COLOR },
            { "src_alpha", osg::BlendFunc::SRC_ALPHA },
            { "src_alpha_saturate", osg::BlendFunc::SRC_ALPHA_SATURATE },
            { "src_color", osg::BlendFunc::SRC_COLOR },
            { "constant_color", osg::BlendFunc::CONSTANT_COLOR },
            { "one_minus_constant_color", osg::BlendFunc::ONE_MINUS_CONSTANT_COLOR },
            { "constant_alpha", osg::BlendFunc::CONSTANT_ALPHA },
            { "one_minus_constant_alpha", osg::BlendFunc::ONE_MINUS_CONSTANT_ALPHA },
            { "zero", osg::BlendFunc::ZERO },
        } };

        constexpr std::array<std::pair<std::string_view, osg::BlendEquation::Equation>, 8> BlendEquation = { {
            { "rgba_min", osg::BlendEquation::RGBA_MIN },
            { "rgba_max", osg::BlendEquation::RGBA_MAX },
            { "alpha_min", osg::BlendEquation::ALPHA_MIN },
            { "alpha_max", osg::BlendEquation::ALPHA_MAX },
            { "logic_op", osg::BlendEquation::LOGIC_OP },
            { "add", osg::BlendEquation::FUNC_ADD },
            { "subtract", osg::BlendEquation::FUNC_SUBTRACT },
            { "reverse_subtract", osg::BlendEquation::FUNC_REVERSE_SUBTRACT },
        } };
    }
}

#endif
