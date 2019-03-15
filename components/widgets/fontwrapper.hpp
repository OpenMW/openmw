#ifndef OPENMW_WIDGETS_WRAPPER_H
#define OPENMW_WIDGETS_WRAPPER_H

#include "widgets.hpp"

#include <components/settings/settings.hpp>

namespace Gui
{
    template<class T>
    class FontWrapper : public T
    {
    public:
        virtual void setFontName(const std::string& name)
        {
            T::setFontName(name);
            T::setPropertyOverride ("FontHeight", getFontSize());
        }

    protected:
        virtual void setPropertyOverride(const std::string& _key, const std::string& _value)
        {
            T::setPropertyOverride (_key, _value);

            // There is a bug in MyGUI: when it initializes the FontName property, it reset the font height.
            // We should restore it.
            if (_key == "FontName")
            {
                T::setPropertyOverride ("FontHeight", getFontSize());
            }
        }

    private:
        static int clamp(const int& value, const int& lowBound, const int& highBound)
        {
            return std::min(std::max(lowBound, value), highBound);
        }

        std::string getFontSize()
        {
            // Note: we can not use the WindowManager here, so there is a code duplication a bit.
            static const std::string fontSize = std::to_string(clamp(Settings::Manager::getInt("font size", "GUI"), 12, 20));
            return fontSize;
        }
    };
}

#endif
