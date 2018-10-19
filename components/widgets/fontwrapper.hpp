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
            T::setPropertyOverride ("FontHeight", mFontSize);
        }

    protected:
        FontWrapper()
        {
            // Note: we can not use the WindowManager here, so there is a code duplication a bit.
            int fontSize = Settings::Manager::getInt("font size", "GUI");
            fontSize = std::min(std::max(12, fontSize), 20);
            mFontSize = std::to_string(fontSize);
        }

        virtual void setPropertyOverride(const std::string& _key, const std::string& _value)
        {
            T::setPropertyOverride (_key, _value);

            // There is a bug in MyGUI: when it initializes the FontName property, it reset the font height.
            // We should restore it.
            if (_key == "FontName")
            {
                T::setPropertyOverride ("FontHeight", mFontSize);
            }
        }

        std::string mFontSize;
    };
}

#endif
