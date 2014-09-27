#ifndef OPENMW_MYGUI_RESOURCE_PLUGIN_H
#define OPENMW_MYGUI_RESOURCE_PLUGIN_H

#include <MyGUI_Plugin.h>
#include <MyGUI_UString.h>

namespace MyGUIPlugin
{

    /**
     * @brief MyGUI plugin used to register Morrowind resources, custom widgets used in OpenMW, and load Morrowind fonts.
     * @paragraph The plugin isn't used in OpenMW itself, but it is useful with the standalone MyGUI tools. To use it,
     *            change EditorPlugin.xml in Media/Tools/LayoutEditor/EditorPlugin.xml and add an entry for this plugin.
     */
    class ResourcePlugin : public MyGUI::IPlugin
    {
        /*!	Get the name of the plugin.
            @remarks An implementation must be supplied for this method to uniquely
            identify the plugin
        */
        virtual const std::string& getName() const;

        /*!	Perform the plugin initial installation sequence
        */
        virtual void install();

        /*! Perform any tasks the plugin needs to perform on full system
            initialisation.
        */
        virtual void initialize();

        /*!	Perform any tasks the plugin needs to perform when the system is shut down
        */
        virtual void shutdown();

        /*!	Perform the final plugin uninstallation sequence
        */
        virtual void uninstall();

    private:
        void registerResources();
        void registerWidgets();
        void createTransparentBGTexture();

        void onRetrieveTag(const MyGUI::UString& tag, MyGUI::UString& out);

        std::map<std::string, std::string> mFallbackMap;
    };

}

#endif
