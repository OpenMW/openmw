#ifndef OPENMW_COMPONENTS_FX_LEXERTYPES_HPP
#define OPENMW_COMPONENTS_FX_LEXERTYPES_HPP

#include <string_view>
#include <variant>

namespace fx
{
    namespace Lexer
    {
        struct Float
        {
            inline static constexpr std::string_view repr = "float";
            float value = 0.0;
        };
        struct Integer
        {
            inline static constexpr std::string_view repr = "integer";
            int value = 0;
        };
        struct Boolean
        {
            inline static constexpr std::string_view repr = "boolean";
            bool value = false;
        };
        struct Literal
        {
            inline static constexpr std::string_view repr = "literal";
            std::string_view value;
        };
        struct String
        {
            inline static constexpr std::string_view repr = "string";
            std::string_view value;
        };
        struct Shared
        {
            inline static constexpr std::string_view repr = "shared";
        };
        struct Vertex
        {
            inline static constexpr std::string_view repr = "vertex";
        };
        struct Fragment
        {
            inline static constexpr std::string_view repr = "fragment";
        };
        struct Compute
        {
            inline static constexpr std::string_view repr = "compute";
        };
        struct Technique
        {
            inline static constexpr std::string_view repr = "technique";
        };
        struct Render_Target
        {
            inline static constexpr std::string_view repr = "render_target";
        };
        struct Sampler_1D
        {
            inline static constexpr std::string_view repr = "sampler_1d";
        };
        struct Sampler_2D
        {
            inline static constexpr std::string_view repr = "sampler_2d";
        };
        struct Sampler_3D
        {
            inline static constexpr std::string_view repr = "sampler_3d";
        };
        struct Uniform_Bool
        {
            inline static constexpr std::string_view repr = "uniform_bool";
        };
        struct Uniform_Float
        {
            inline static constexpr std::string_view repr = "uniform_float";
        };
        struct Uniform_Int
        {
            inline static constexpr std::string_view repr = "uniform_int";
        };
        struct Uniform_Vec2
        {
            inline static constexpr std::string_view repr = "uniform_vec2";
        };
        struct Uniform_Vec3
        {
            inline static constexpr std::string_view repr = "uniform_vec3";
        };
        struct Uniform_Vec4
        {
            inline static constexpr std::string_view repr = "uniform_vec4";
        };
        struct Eof
        {
            inline static constexpr std::string_view repr = "eof";
        };
        struct Equal
        {
            inline static constexpr std::string_view repr = "equal";
        };
        struct Open_bracket
        {
            inline static constexpr std::string_view repr = "open_bracket";
        };
        struct Close_bracket
        {
            inline static constexpr std::string_view repr = "close_bracket";
        };
        struct Open_Parenthesis
        {
            inline static constexpr std::string_view repr = "open_parenthesis";
        };
        struct Close_Parenthesis
        {
            inline static constexpr std::string_view repr = "close_parenthesis";
        };
        struct Quote
        {
            inline static constexpr std::string_view repr = "quote";
        };
        struct SemiColon
        {
            inline static constexpr std::string_view repr = "semicolon";
        };
        struct Comma
        {
            inline static constexpr std::string_view repr = "comma";
        };
        struct VBar
        {
            inline static constexpr std::string_view repr = "vbar";
        };
        struct Colon
        {
            inline static constexpr std::string_view repr = "colon";
        };
        struct True
        {
            inline static constexpr std::string_view repr = "true";
        };
        struct False
        {
            inline static constexpr std::string_view repr = "false";
        };
        struct Vec2
        {
            inline static constexpr std::string_view repr = "vec2";
        };
        struct Vec3
        {
            inline static constexpr std::string_view repr = "vec3";
        };
        struct Vec4
        {
            inline static constexpr std::string_view repr = "vec4";
        };

        using Token = std::variant<Float, Integer, Boolean, String, Literal, Equal, Open_bracket, Close_bracket,
            Open_Parenthesis, Close_Parenthesis, Quote, SemiColon, Comma, VBar, Colon, Shared, Technique, Render_Target,
            Vertex, Fragment, Compute, Sampler_1D, Sampler_2D, Sampler_3D, Uniform_Bool, Uniform_Float, Uniform_Int,
            Uniform_Vec2, Uniform_Vec3, Uniform_Vec4, True, False, Vec2, Vec3, Vec4, Eof>;
    }
}

#endif
