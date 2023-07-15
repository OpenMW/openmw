#ifndef OPENMW_WIDGETS_WRAPPER_H
#define OPENMW_WIDGETS_WRAPPER_H

#include <MyGUI_Prerequest.h>
#include <components/settings/settings.hpp>

#include <algorithm>

namespace Gui
{
    template <class T>
    class FontWrapper : public T
    {
#if MYGUI_VERSION <= MYGUI_DEFINE_VERSION(3, 4, 2)
    public:
        void setFontName(const std::string& name) override
        {
            T::setFontName(name);
            T::setPropertyOverride("FontHeight", getFontSize());
        }

    protected:
        void setPropertyOverride(const std::string& _key, const std::string& _value) override
        {
            T::setPropertyOverride(_key, _value);

            // https://github.com/MyGUI/mygui/issues/113
            // There is a bug in MyGUI: when it initializes the FontName property, it reset the font height.
            // We should restore it.
            if (_key == "FontName")
            {
                T::setPropertyOverride("FontHeight", getFontSize());
            }
        }

    private:
        const std::string& getFontSize()
        {
            // Note: we can not use the FontLoader here, so there is a code duplication a bit.
            static const std::string fontSize
                = std::to_string(std::clamp(Settings::Manager::getInt("font size", "GUI"), 12, 18));
            return fontSize;
        }
#endif
    };
}

#endif
