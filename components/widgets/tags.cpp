#include "tags.hpp"

#include <components/settings/values.hpp>
#include <components/fallback/fallback.hpp>

#include <MyGUI_Colour.h>

namespace Gui
{

    bool replaceTag(std::string_view tag, MyGUI::UString& out)
    {
        std::string_view fontcolour = "fontcolour=";
        std::string_view fontcolourhtml = "fontcolourhtml=";
        std::string_view fontcolouroptional = "fontcolouroptional=";

        if (tag.starts_with(fontcolour))
        {
            std::string fallbackName = "FontColor_color_";
            fallbackName += tag.substr(fontcolour.length());
            std::string_view str = Fallback::Map::getString(fallbackName);
            if (str.empty())
                throw std::runtime_error("Unable to map setting to value: " + fallbackName);

            std::string ret[3];
            unsigned int j = 0;
            for (unsigned int i = 0; i < str.length(); ++i)
            {
                if (str[i] == ',')
                    j++;
                else if (str[i] != ' ')
                    ret[j] += str[i];
            }
            MyGUI::Colour col(MyGUI::utility::parseInt(ret[0]) / 255.f, MyGUI::utility::parseInt(ret[1]) / 255.f,
                MyGUI::utility::parseInt(ret[2]) / 255.f);
            out = col.print();
            return true;
        }
        else if (tag.starts_with(fontcolourhtml))
        {
            std::string fallbackName = "FontColor_color_";
            fallbackName += tag.substr(fontcolourhtml.length());
            std::string_view str = Fallback::Map::getString(fallbackName);
            if (str.empty())
                throw std::runtime_error("Unknown fallback name: " + fallbackName);

            std::string ret[3];
            unsigned int j = 0;
            for (unsigned int i = 0; i < str.length(); ++i)
            {
                if (str[i] == ',')
                    j++;
                else if (str[i] != ' ')
                    ret[j] += str[i];
            }
            std::stringstream html;
            html << "#" << std::hex << MyGUI::utility::parseInt(ret[0]) << MyGUI::utility::parseInt(ret[1])
                 << MyGUI::utility::parseInt(ret[2]);
            out = html.str();
            return true;
        }
        else if (tag.starts_with(fontcolouroptional))
        {
            std::string_view category = "GUI";
            std::string colortag = "";
            colortag += tag.substr(fontcolouroptional.length());
            std::string str = Settings::Manager::getString(colortag, category);
            if (str.empty())
                throw std::runtime_error("Unable to map setting to value: " + colortag);

            std::string ret[4];
            unsigned int j = 0;
            for (unsigned int i = 0; i < str.length(); ++i)
            {
                if (str[i] == ' ')
                    j++;
                else if (str[i] != ' ')
                    ret[j] += str[i];
            }
            MyGUI::Colour col(MyGUI::utility::parseFloat(ret[0]), MyGUI::utility::parseFloat(ret[1]),
                MyGUI::utility::parseFloat(ret[2]), MyGUI::utility::parseFloat(ret[3]));
            out = col.print();
            return true;
        }
        return false;
    }

}
