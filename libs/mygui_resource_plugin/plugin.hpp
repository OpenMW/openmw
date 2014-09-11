#ifndef OPENMW_MYGUI_RESOURCE_PLUGIN_H
#define OPENMW_MYGUI_RESOURCE_PLUGIN_H

#include <MyGUI_Plugin.h>

namespace MyGUI
{

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
    };

}

#endif
