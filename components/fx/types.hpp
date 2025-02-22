#ifndef OPENMW_COMPONENTS_FX_TYPES_HPP
#define OPENMW_COMPONENTS_FX_TYPES_HPP

#include <optional>
#include <variant>

#include <osg/Texture2D>
#include <osg/Uniform>

#include <components/debug/debuglog.hpp>
#include <components/misc/strings/format.hpp>
#include <components/sceneutil/depth.hpp>
#include <components/settings/shadermanager.hpp>

namespace fx
{
    namespace Types
    {
        struct SizeProxy
        {
            std::optional<float> mWidthRatio;
            std::optional<float> mHeightRatio;
            std::optional<int> mWidth;
            std::optional<int> mHeight;

            std::tuple<int, int> get(int width, int height) const
            {
                int scaledWidth = width;
                int scaledHeight = height;

                if (mWidthRatio)
                    scaledWidth = width * mWidthRatio.value();
                else if (mWidth)
                    scaledWidth = mWidth.value();

                if (mHeightRatio > 0.f)
                    scaledHeight = height * mHeightRatio.value();
                else if (mHeight)
                    scaledHeight = mHeight.value();

                return std::make_tuple(scaledWidth, scaledHeight);
            }
        };

        struct RenderTarget
        {
            osg::ref_ptr<osg::Texture2D> mTarget = new osg::Texture2D;
            SizeProxy mSize;
            bool mMipMap = false;
            osg::Vec4f mClearColor = osg::Vec4f(0.0, 0.0, 0.0, 1.0);
        };

        template <class T>
        struct Choice
        {
            std::string mLabel;
            T mValue;
        };

        template <class T>
        struct Uniform
        {
            std::optional<T> mValue;
            std::optional<std::vector<T>> mArray;

            T mDefault = {};
            T mMin = std::numeric_limits<T>::lowest();
            T mMax = std::numeric_limits<T>::max();

            std::vector<Choice<T>> mChoices;

            using value_type = T;

            bool isArray() const { return mArray.has_value(); }

            const std::vector<T>& getArray() const { return *mArray; }

            T getValue() const { return mValue.value_or(mDefault); }
        };

        using Uniform_t = std::variant<Uniform<osg::Vec2f>, Uniform<osg::Vec3f>, Uniform<osg::Vec4f>, Uniform<bool>,
            Uniform<float>, Uniform<int>>;

        enum SamplerType
        {
            Texture_1D,
            Texture_2D,
            Texture_3D
        };

        struct UniformBase
        {
            std::string mName;
            std::string mDisplayName;
            std::string mHeader;
            std::string mTechniqueName;
            std::string mDescription;

            bool mStatic = true;
            std::optional<SamplerType> mSamplerType = std::nullopt;
            double mStep = 1.0;

            Uniform_t mData;

            template <class T>
            T getValue() const
            {
                auto value = Settings::ShaderManager::get().getValue<T>(mTechniqueName, mName);

                return value.value_or(std::get<Uniform<T>>(mData).getValue());
            }

            size_t getNumElements() const
            {
                return std::visit(
                    [&](auto&& arg) {
                        ;
                        return arg.isArray() ? arg.getArray().size() : 1;
                    },
                    mData);
            }

            template <class T>
            T getMin() const
            {
                return std::get<Uniform<T>>(mData).mMin;
            }

            template <class T>
            T getMax() const
            {
                return std::get<Uniform<T>>(mData).mMax;
            }

            template <class T>
            T getDefault() const
            {
                return std::get<Uniform<T>>(mData).mDefault;
            }

            template <class T>
            void setValue(const T& value)
            {
                std::visit(
                    [&, value](auto&& arg) {
                        using U = typename std::decay_t<decltype(arg)>::value_type;

                        if constexpr (std::is_same_v<T, U>)
                        {
                            arg.mValue = value;

                            Settings::ShaderManager::get().setValue(mTechniqueName, mName, value);
                        }
                        else
                        {
                            Log(Debug::Warning) << "Attempting to set uniform '" << mName << "' with wrong type";
                        }
                    },
                    mData);
            }

            template <class T, class A>
            void setValue(const std::vector<T, A>& value)
            {
                std::visit(
                    [&, value](auto&& arg) {
                        using U = typename std::decay_t<decltype(arg)>::value_type;

                        if (!arg.isArray() || arg.getArray().size() != value.size())
                        {
                            Log(Debug::Error)
                                << "Attempting to set uniform array '" << mName << "' with mismatching array sizes";
                            return;
                        }

                        if constexpr (std::is_same_v<T, U>)
                        {
                            arg.mArray = value;
                            Settings::ShaderManager::get().setValue(mTechniqueName, mName, value);
                        }
                        else
                            Log(Debug::Warning) << "Attempting to set uniform array '" << mName << "' with wrong type";
                    },
                    mData);
            }

            void setUniform(osg::Uniform* uniform)
            {
                auto type = getType();
                if (!type || type.value() != uniform->getType())
                    return;

                std::visit(
                    [&](auto&& arg) {
                        if (arg.isArray())
                        {
                            for (size_t i = 0; i < arg.getArray().size(); ++i)
                                uniform->setElement(i, arg.getArray()[i]);
                            uniform->dirty();
                        }
                        else
                            uniform->set(arg.getValue());
                    },
                    mData);
            }

            std::optional<osg::Uniform::Type> getType() const
            {
                return std::visit(
                    [](auto&& arg) -> std::optional<osg::Uniform::Type> {
                        using T = typename std::decay_t<decltype(arg)>::value_type;

                        if constexpr (std::is_same_v<T, osg::Vec2f>)
                            return osg::Uniform::FLOAT_VEC2;
                        else if constexpr (std::is_same_v<T, osg::Vec3f>)
                            return osg::Uniform::FLOAT_VEC3;
                        else if constexpr (std::is_same_v<T, osg::Vec4f>)
                            return osg::Uniform::FLOAT_VEC4;
                        else if constexpr (std::is_same_v<T, float>)
                            return osg::Uniform::FLOAT;
                        else if constexpr (std::is_same_v<T, int>)
                            return osg::Uniform::INT;
                        else if constexpr (std::is_same_v<T, bool>)
                            return osg::Uniform::BOOL;

                        return std::nullopt;
                    },
                    mData);
            }

            std::optional<std::string> getGLSL()
            {
                if (mSamplerType)
                {
                    switch (mSamplerType.value())
                    {
                        case Texture_1D:
                            return Misc::StringUtils::format("uniform sampler1D %s;", mName);
                        case Texture_2D:
                            return Misc::StringUtils::format("uniform sampler2D %s;", mName);
                        case Texture_3D:
                            return Misc::StringUtils::format("uniform sampler3D %s;", mName);
                    }
                }

                return std::visit(
                    [&](auto&& arg) -> std::optional<std::string> {
                        using T = typename std::decay_t<decltype(arg)>::value_type;

                        auto value = arg.getValue();

                        const bool useUniform = arg.isArray()
                            || (Settings::ShaderManager::get().getMode() == Settings::ShaderManager::Mode::Debug
                                || mStatic == false);
                        const std::string uname = arg.isArray()
                            ? Misc::StringUtils::format("%s[%zu]", mName, arg.getArray().size())
                            : mName;

                        if constexpr (std::is_same_v<T, osg::Vec2f>)
                        {
                            if (useUniform)
                                return Misc::StringUtils::format("uniform vec2 %s;", uname);

                            return Misc::StringUtils::format("const vec2 %s=vec2(%f,%f);", mName, value[0], value[1]);
                        }
                        else if constexpr (std::is_same_v<T, osg::Vec3f>)
                        {
                            if (useUniform)
                                return Misc::StringUtils::format("uniform vec3 %s;", uname);

                            return Misc::StringUtils::format(
                                "const vec3 %s=vec3(%f,%f,%f);", mName, value[0], value[1], value[2]);
                        }
                        else if constexpr (std::is_same_v<T, osg::Vec4f>)
                        {
                            if (useUniform)
                                return Misc::StringUtils::format("uniform vec4 %s;", uname);

                            return Misc::StringUtils::format(
                                "const vec4 %s=vec4(%f,%f,%f,%f);", mName, value[0], value[1], value[2], value[3]);
                        }
                        else if constexpr (std::is_same_v<T, float>)
                        {
                            if (useUniform)
                                return Misc::StringUtils::format("uniform float %s;", uname);

                            return Misc::StringUtils::format("const float %s=%f;", mName, value);
                        }
                        else if constexpr (std::is_same_v<T, int>)
                        {
                            if (useUniform)
                                return Misc::StringUtils::format("uniform int %s;", uname);

                            return Misc::StringUtils::format("const int %s=%i;", mName, value);
                        }
                        else if constexpr (std::is_same_v<T, bool>)
                        {
                            if (useUniform)
                                return Misc::StringUtils::format("uniform bool %s;", uname);

                            return Misc::StringUtils::format("const bool %s=%s;", mName, value ? "true" : "false");
                        }

                        return std::nullopt;
                    },
                    mData);
            }
        };
    }
}

#endif
