#include "gui.hpp"

#include <MyGUI_InputManager.h>

#include "../luamanager.hpp"
#include "../util.hpp"

#include "../../mwbase/environment.hpp"
#include "../../mwbase/windowmanager.hpp"

namespace MWLua
{
    void bindTES3GUIFunctions()
    {
        auto stateHandle = LuaManager::getInstance().getThreadSafeStateHandle();
        sol::state& state = stateHandle.state;

        state["omw"]["messageBox"] = [](sol::object param, sol::optional<sol::variadic_args> va)
        {
            auto& luaManager = LuaManager::getInstance();
            auto stateHandle = luaManager.getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;

            if (param.is<std::string>())
            {
                std::string message = state["string"]["format"](param, va);
                MWBase::Environment::get().getWindowManager()->messageBox(message);
                return 0;
            }
            else if (param.get_type() == sol::type::table)
            {
                sol::table params = param;

                std::vector<std::string> buttonText;

                //  Get the parameters out of the table and into the table.
                std::string message = params["message"];
                sol::optional<sol::table> maybeButtons = params["buttons"];
                if (maybeButtons && maybeButtons.value().size() > 0)
                {
                    sol::table buttons = maybeButtons.value();
                    size_t size = std::min(buttons.size(), size_t(32));
                    for (size_t i = 0; i < size; i++)
                    {
                        std::string result = buttons[i + 1];
                        if (result.empty())
                        {
                            break;
                        }

                        buttonText.push_back(result);
                    }
                }

                // No buttons, do a normal popup.
                else
                {
                    MWBase::Environment::get().getWindowManager()->messageBox(message);
                    return 0;
                }

                // FIXME: support callbacks to manage messagebox
                // Set up our event callback.
                //LuaManager::getInstance().setButtonPressedCallback(params["callback"]);

                MWBase::Environment::get().getWindowManager()->interactiveMessageBox(message, buttonText);
            }
            else
            {
                sol::protected_function_result result = state["tostring"](param);
                if (result.valid())
                {
                    sol::optional<const char*> asString = result;
                    if (asString)
                    {
                        MWBase::Environment::get().getWindowManager()->messageBox(asString.value());
                        return 0;
                    }
                }
                throw std::runtime_error("owm.messageBox: Unable to convert parameter to string.");
            }
            return 0;
        };

        state["omw"]["showRepairServiceMenu"] = [](sol::table params)
        {
            MWWorld::Ptr ptr = getOptionalParamExecutionReference(params);
            if (ptr.isEmpty())
            {
                throw std::invalid_argument("Invalid reference parameter provided.");
            }

            MWBase::Environment::get().getWindowManager()->pushGuiMode(MWGui::GM_MerchantRepair, ptr);
        };

        state["omw"]["fadeIn"] = [](float duration)
        {
            MWBase::Environment::get().getWindowManager()->fadeScreenIn(duration, false);
        };

        state["omw"]["fadeOut"] = [](float duration)
        {
            MWBase::Environment::get().getWindowManager()->fadeScreenOut(duration, false);
        };

        state["omw"]["fadeTo"] = [](sol::optional<sol::table> params)
        {
            float alpha = getOptionalParam(params, "value", 1.0f);
            float duration = getOptionalParam(params, "duration", 1.0f);
            MWBase::Environment::get().getWindowManager()->fadeScreenTo(static_cast<int>(alpha*100), duration, false);
        };

        state["omw"]["getCursorPosition"] = []() -> sol::object
        {
            MyGUI::IntPoint cursorPosition = MyGUI::InputManager::getInstance().getMousePosition();
            auto& luaManager = LuaManager::getInstance();
            auto stateHandle = luaManager.getThreadSafeStateHandle();
            sol::state& state = stateHandle.state;
            sol::table results = state.create_table();
            results["x"] = int(cursorPosition.left);
            results["y"] = int(cursorPosition.top);
            return results;
        };
    }
}
