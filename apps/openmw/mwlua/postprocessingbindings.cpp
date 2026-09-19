#include "postprocessingbindings.hpp"

#include "MyGUI_LanguageManager.h"

#include <type_traits>

#include <components/lua/util.hpp>
#include <components/misc/convert.hpp>

#include "../mwbase/environment.hpp"
#include "../mwbase/world.hpp"
#include "../mwrender/postprocessor.hpp"

#include "context.hpp"
#include "luamanagerimp.hpp"

#include <format>

namespace
{
    std::string getLocalizedMyGUIString(std::string_view unlocalized)
    {
        return MyGUI::LanguageManager::getInstance().replaceTags(MyGUI::UString(unlocalized)).asUTF8();
    }
}

namespace MWLua
{
    struct Shader;
}

namespace sol
{
    template <>
    struct is_automagical<MWLua::Shader> : std::false_type
    {
    };
}

namespace MWLua
{
    struct Shader
    {
        std::shared_ptr<Fx::Technique> mShader;

        Shader(std::shared_ptr<Fx::Technique> shader)
            : mShader(std::move(shader))
        {
        }

        std::string toString() const
        {
            if (!mShader)
                return "Shader(nil)";

            return std::format("Shader({}, {})", mShader->getName(), mShader->getFileName().value());
        }

        enum
        {
            Action_None,
            Action_Enable,
            Action_Disable
        } mQueuedAction
            = Action_None;
    };

    // Lua vectors are glm; shader uniforms are still osg.
    template <class T>
    struct UniformType
    {
        using Type = T;
    };
    template <>
    struct UniformType<glm::vec2>
    {
        using Type = osg::Vec2f;
    };
    template <>
    struct UniformType<glm::vec3>
    {
        using Type = osg::Vec3f;
    };
    template <>
    struct UniformType<glm::vec4>
    {
        using Type = osg::Vec4f;
    };

    template <class T>
    typename UniformType<T>::Type toUniform(const T& value)
    {
        if constexpr (std::is_same_v<T, typename UniformType<T>::Type>)
            return value;
        else
            return Misc::Convert::toOsg(value);
    }

    template <class T>
    auto getSetter(const Context& context)
    {
        return [context](const Shader& shader, const std::string& name, const T& value) {
            context.mLuaManager->addAction(
                [=] {
                    MWBase::Environment::get().getWorld()->getPostProcessor()->setUniform(
                        shader.mShader, name, toUniform(value));
                },
                "SetUniformShaderAction");
        };
    }

    template <class T>
    auto getArraySetter(const Context& context)
    {
        return [context](const Shader& shader, const std::string& name, const sol::table& table) {
            auto targetSize
                = MWBase::Environment::get().getWorld()->getPostProcessor()->getUniformSize(shader.mShader, name);

            if (!targetSize.has_value())
                throw std::runtime_error(std::format("Failed setting uniform array '{}'", name));

            if (*targetSize != table.size())
                throw std::runtime_error(
                    std::format("Mismatching uniform array size, got {} expected {}", table.size(), *targetSize));

            std::vector<typename UniformType<T>::Type> values;
            values.reserve(*targetSize);

            for (size_t i = 0; i < *targetSize; ++i)
            {
                sol::object obj = table[LuaUtil::toLuaIndex(i)];
                if (!obj.is<T>())
                    throw std::runtime_error("Invalid type for uniform array");
                values.push_back(toUniform(obj.as<T>()));
            }

            context.mLuaManager->addAction(
                [shader, name, values = std::move(values)] {
                    MWBase::Environment::get().getWorld()->getPostProcessor()->setUniform(shader.mShader, name, values);
                },
                "SetUniformShaderAction");
        };
    }

    sol::table initPostprocessingPackage(const Context& context)
    {
        sol::state_view lua = context.sol();
        sol::table api(lua, sol::create);

        {
            sol::usertype<Shader> shader = lua.new_usertype<Shader>("Shader");
            shader[sol::meta_function::to_string] = [](const Shader& self) { return self.toString(); };

            shader["enable"] = [context](Shader& self, sol::optional<int> optPos) {
                std::optional<int> pos = std::nullopt;
                if (optPos)
                    pos = optPos.value();

                if (self.mShader && self.mShader->isValid())
                    self.mQueuedAction = Shader::Action_Enable;

                context.mLuaManager->addAction([=, &self] {
                    self.mQueuedAction = Shader::Action_None;

                    if (MWBase::Environment::get().getWorld()->getPostProcessor()->enableTechnique(self.mShader, pos)
                        == MWRender::PostProcessor::Status_Error)
                        throw std::runtime_error(std::format("Failed enabling shader '{}'", self.mShader->getName()));
                });
            };

            shader["disable"] = [context](Shader& self) {
                self.mQueuedAction = Shader::Action_Disable;

                context.mLuaManager->addAction([&] {
                    self.mQueuedAction = Shader::Action_None;

                    if (MWBase::Environment::get().getWorld()->getPostProcessor()->disableTechnique(self.mShader)
                        == MWRender::PostProcessor::Status_Error)
                        throw std::runtime_error(std::format("Failed disabling shader '{}'", self.mShader->getName()));
                });
            };

            shader["isEnabled"] = [](const Shader& self) {
                if (self.mQueuedAction == Shader::Action_Enable)
                    return true;
                else if (self.mQueuedAction == Shader::Action_Disable)
                    return false;
                return MWBase::Environment::get().getWorld()->getPostProcessor()->isTechniqueEnabled(self.mShader);
            };

            shader["name"] = sol::readonly_property(
                [](const Shader& self) { return getLocalizedMyGUIString(self.mShader->getName()); });
            shader["author"] = sol::readonly_property(
                [](const Shader& self) { return getLocalizedMyGUIString(self.mShader->getAuthor()); });
            shader["description"] = sol::readonly_property(
                [](const Shader& self) { return getLocalizedMyGUIString(self.mShader->getDescription()); });
            shader["version"] = sol::readonly_property(
                [](const Shader& self) { return getLocalizedMyGUIString(self.mShader->getVersion()); });

            shader["setBool"] = getSetter<bool>(context);
            shader["setFloat"] = getSetter<float>(context);
            shader["setInt"] = getSetter<int>(context);
            shader["setVector2"] = getSetter<glm::vec2>(context);
            shader["setVector3"] = getSetter<glm::vec3>(context);
            shader["setVector4"] = getSetter<glm::vec4>(context);

            shader["setFloatArray"] = getArraySetter<float>(context);
            shader["setIntArray"] = getArraySetter<int>(context);
            shader["setVector2Array"] = getArraySetter<glm::vec2>(context);
            shader["setVector3Array"] = getArraySetter<glm::vec3>(context);
            shader["setVector4Array"] = getArraySetter<glm::vec4>(context);
        }

        api["load"] = [](const std::string& name) {
            Shader shader{ MWBase::Environment::get().getWorld()->getPostProcessor()->loadTechnique(name, false) };

            if (!shader.mShader || !shader.mShader->isValid())
                throw std::runtime_error(std::format("Failed loading shader '{}'", name));

            return shader;
        };

        api["getChain"] = [context]() {
            sol::table chain(context.sol(), sol::create);

            for (const auto& shader : MWBase::Environment::get().getWorld()->getPostProcessor()->getChain())
            {
                // Don't expose internal shaders to the API, they should be invisible to the user
                if (shader->getInternal())
                    continue;
                chain.add(Shader(shader));
            }

            return chain;
        };

        return LuaUtil::makeReadOnly(api);
    }

}
