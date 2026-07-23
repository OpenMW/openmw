#ifndef OPENMW_COMPONENTS_FX_LEXERTYPES_HPP
#define OPENMW_COMPONENTS_FX_LEXERTYPES_HPP

#include <string_view>
#include <variant>

namespace Fx
{
    namespace Lexer
    {
        struct Float
        {
            inline static constexpr std::string_view mRepresentation = "float";
            float mValue = 0.0;
        };
        struct Integer
        {
            inline static constexpr std::string_view mRepresentation = "integer";
            int mValue = 0;
        };
        struct Boolean
        {
            inline static constexpr std::string_view mRepresentation = "boolean";
            bool mValue = false;
        };
        struct Literal
        {
            inline static constexpr std::string_view mRepresentation = "literal";
            std::string_view mValue;
        };
        struct String
        {
            inline static constexpr std::string_view mRepresentation = "string";
            std::string_view mValue;
        };
        struct Shared
        {
            inline static constexpr std::string_view mRepresentation = "shared";
        };
        struct Vertex
        {
            inline static constexpr std::string_view mRepresentation = "vertex";
        };
        struct Fragment
        {
            inline static constexpr std::string_view mRepresentation = "fragment";
        };
        struct Compute
        {
            inline static constexpr std::string_view mRepresentation = "compute";
        };
        struct Technique
        {
            inline static constexpr std::string_view mRepresentation = "technique";
        };
        struct Render_Target
        {
            inline static constexpr std::string_view mRepresentation = "render_target";
        };
        struct Sampler_1D
        {
            inline static constexpr std::string_view mRepresentation = "sampler_1d";
        };
        struct Sampler_2D
        {
            inline static constexpr std::string_view mRepresentation = "sampler_2d";
        };
        struct Sampler_3D
        {
            inline static constexpr std::string_view mRepresentation = "sampler_3d";
        };
        struct Uniform_Bool
        {
            inline static constexpr std::string_view mRepresentation = "uniform_bool";
        };
        struct Uniform_Float
        {
            inline static constexpr std::string_view mRepresentation = "uniform_float";
        };
        struct Uniform_Int
        {
            inline static constexpr std::string_view mRepresentation = "uniform_int";
        };
        struct Uniform_Vec2
        {
            inline static constexpr std::string_view mRepresentation = "uniform_vec2";
        };
        struct Uniform_Vec3
        {
            inline static constexpr std::string_view mRepresentation = "uniform_vec3";
        };
        struct Uniform_Vec4
        {
            inline static constexpr std::string_view mRepresentation = "uniform_vec4";
        };
        struct Eof
        {
            inline static constexpr std::string_view mRepresentation = "eof";
        };
        struct Equal
        {
            inline static constexpr std::string_view mRepresentation = "equal";
        };
        struct Open_bracket
        {
            inline static constexpr std::string_view mRepresentation = "open_bracket";
        };
        struct Close_bracket
        {
            inline static constexpr std::string_view mRepresentation = "close_bracket";
        };
        struct Open_Parenthesis
        {
            inline static constexpr std::string_view mRepresentation = "open_parenthesis";
        };
        struct Close_Parenthesis
        {
            inline static constexpr std::string_view mRepresentation = "close_parenthesis";
        };
        struct Quote
        {
            inline static constexpr std::string_view mRepresentation = "quote";
        };
        struct SemiColon
        {
            inline static constexpr std::string_view mRepresentation = "semicolon";
        };
        struct Comma
        {
            inline static constexpr std::string_view mRepresentation = "comma";
        };
        struct VBar
        {
            inline static constexpr std::string_view mRepresentation = "vbar";
        };
        struct Colon
        {
            inline static constexpr std::string_view mRepresentation = "colon";
        };
        struct True
        {
            inline static constexpr std::string_view mRepresentation = "true";
        };
        struct False
        {
            inline static constexpr std::string_view mRepresentation = "false";
        };
        struct Vec2
        {
            inline static constexpr std::string_view mRepresentation = "vec2";
        };
        struct Vec3
        {
            inline static constexpr std::string_view mRepresentation = "vec3";
        };
        struct Vec4
        {
            inline static constexpr std::string_view mRepresentation = "vec4";
        };

        using Token = std::variant<Float, Integer, Boolean, String, Literal, Equal, Open_bracket, Close_bracket,
            Open_Parenthesis, Close_Parenthesis, Quote, SemiColon, Comma, VBar, Colon, Shared, Technique, Render_Target,
            Vertex, Fragment, Compute, Sampler_1D, Sampler_2D, Sampler_3D, Uniform_Bool, Uniform_Float, Uniform_Int,
            Uniform_Vec2, Uniform_Vec3, Uniform_Vec4, True, False, Vec2, Vec3, Vec4, Eof>;
    }
}

#endif
